#!/usr/bin/env node
/*───────────────────────────────────────────────────────────────────────────────
  repo_map.js — harvest project metadata into repo_map.json
  ───────────────────────────────────────────────────────────────────────────────
  • Discovers C/C++ sources (default src/, tests/) using Tree-sitter.
  • Generates a JSON knowledge-graph consumed by meta-build agents.
  • CLI: node repo_map.js [--src DIR]... [--tests DIR] [--out FILE]
           [--exclude GLOB]... [--jobs N]
     – multiple --src flags allowed; defaults: src/, tests/, repo_map.json
  • Emits SHA-256 digest of the JSON on stdout for deterministic CI caching.
  • Requires:    npm i tree-sitter tree-sitter-c fast-glob p-limit
───────────────────────────────────────────────────────────────────────────────*/

'use strict';
const fs          = require('fs');
const path        = require('path');
const crypto      = require('crypto');
const fg          = require('fast-glob');          // robust globbing, cross-platform
const pLimit      = require('p-limit');            // concurrency throttle
const Parser      = require('tree-sitter');
const TREE_SITTER_C_PATH =
  process.env.TREE_SITTER_C_PATH || path.resolve(__dirname, 'tree-sitter-c');
const C           = require(TREE_SITTER_C_PATH);

// ───────────────────────────── Tree-sitter setup ─────────────────────────────
const parser = new Parser();
parser.setLanguage(C);

// ───────────────────────────── CLI argument parse ────────────────────────────
const argv     = process.argv.slice(2);
const srcDirs  = [];
let testsDir   = 'tests';
let outFile    = 'repo_map.json';
const excludes = [];            // glob patterns to ignore
let jobs       = 0;             // 0 = auto (CPU count)

// crude manual parse keeps dependencies zero
for (let i = 0; i < argv.length; ++i) {
  const arg = argv[i];
  function need() {
    if (!argv[i + 1]) { console.error(`missing value for ${arg}`); process.exit(1); }
    return argv[++i];
  }
  switch (arg) {
    case '--src':    case '-s': srcDirs.push(need());          break;
    case '--tests':  case '-t': testsDir = need();             break;
    case '--out':    case '-o': outFile  = need();             break;
    case '--exclude':case '-x': excludes.push(need());         break;
    case '--jobs':   case '-j': jobs = parseInt(need(), 10);   break;
    case '--help':   case '-h':
      console.log(`Usage: node repo_map.js [opts]
  -s, --src DIR       Source dir (repeatable)   [default: src]
  -t, --tests DIR     Tests dir                 [default: tests]
  -o, --out FILE      Output JSON               [default: repo_map.json]
  -x, --exclude GLOB  Ignore (fast-glob syntax) [repeatable]
  -j, --jobs N        Parallel parses (0 = CPU)#`);
      process.exit(0);
    default:
      console.error(`unknown option: ${arg}`); process.exit(1);
  }
}
if (srcDirs.length === 0) srcDirs.push('src');

// ───────────────────────────── Helpers ───────────────────────────────────────
function listCrossFiles() {
  try {
    return fs.readdirSync('cross').filter(f => f.endsWith('.cross'));
  } catch {
    return [];
  }
}

function sha256(data) {
  return crypto.createHash('sha256').update(data).digest('hex');
}

// Tree-sitter visitor
function functionsIn(sourceCode) {
  const tree = parser.parse(sourceCode);
  const fns  = [];
  (function walk(node) {
    if (node.type === 'function_definition') {
      const decl = node.childForFieldName('declarator');
      const id   = decl && decl.descendantsOfType('identifier')[0];
      if (id) fns.push(id.text);
    }
    for (let i = 0; i < node.namedChildCount; ++i) walk(node.namedChild(i));
  })(tree.rootNode);
  return fns;
}

// parse one file
function parseFile(fp) {
  const source = fs.readFileSync(fp, 'utf8');
  return { functions: functionsIn(source) };
}

// ───────────────────────────── Main scan phase (parallel) ────────────────────
const limit   = pLimit(jobs || require('os').cpus().length);
const files   = {};                    // fp → {functions: [...]}

(async () => {
  const patterns     = srcDirs.concat(testsDir).map(d => `${d}/**/*.c`);
  const ignore       = ['**/build/**', '**/vendor/**', ...excludes];
  const candidates   = await fg(patterns, { ignore, onlyFiles: true, unique: true });

  await Promise.all(candidates.map(fp =>
    limit(async () => { files[fp] = parseFile(fp); })
  ));

  // ───────────────────────────── Assemble repo map ───────────────────────────
  const map = {
    generated_at : new Date().toISOString(),
    build_system : 'meson',
    cross_files  : listCrossFiles(),
    toolchains   : listCrossFiles().map(f => f.replace('.cross', '')),
    src_roots    : srcDirs,
    tests_dir    : testsDir,
    test_suites  : fs.existsSync(testsDir)
                    ? fs.readdirSync(testsDir).filter(f => f.endsWith('.c'))
                    : [],
    files,
  };

  const json = JSON.stringify(map, null, 2);
  fs.writeFileSync(outFile, json);

  // CI can key cache artefacts on this digest
  console.log(`repo_map written to ${outFile}`);
  console.log(`SHA-256: ${sha256(json)}`);
})().catch(err => { console.error(err); process.exit(1); });
