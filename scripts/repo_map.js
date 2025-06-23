#!/usr/bin/env node
/*───────────────────────────────────────────────────────────────────────────────
  repo_map.js — harvest repository metadata → repo_map.json
────────────────────────────────────────────────────────────────────────────────
  • Recursively discovers C sources (default src/, tests/) with fast-glob
    and Tree-sitter-C, then extracts top-level function names.
  • CLI:
        node repo_map.js [--src DIR]... [--tests DIR]      \
                         [--cross DIR] [--out FILE]        \
                         [--exclude GLOB]... [--jobs N]

        -s/--src      repeatable source roots     (def: src)
        -t/--tests    single tests dir            (def: tests)
        -c/--cross    Meson cross-file dir        (def: cross or $CROSS_DIR)
        -o/--out      output JSON filename        (def: repo_map.json)
        -x/--exclude  glob patterns to ignore     (repeatable)
        -j/--jobs     parallel parse workers      (0 = auto CPU count)
        -h/--help     usage

  • Emits SHA-256 digest of the generated JSON for CI cache keys.
  • Requires:  npm i tree-sitter tree-sitter-c fast-glob p-limit
───────────────────────────────────────────────────────────────────────────────*/

'use strict';

/* ── 0 · Imports ─────────────────────────────────────────────────────────── */
const fs        = require('fs');
const path      = require('path');                                     // Node path utils
const crypto    = require('crypto');                                   // SHA-256 :contentReference[oaicite:0]{index=0}
const fg        = require('fast-glob');                                // globbing :contentReference[oaicite:1]{index=1}
const pLimit    = require('p-limit');                                  // concurrency :contentReference[oaicite:2]{index=2}
const Parser    = require('tree-sitter');                              // syntax trees :contentReference[oaicite:3]{index=3}
const C         = require('tree-sitter-c');                            // C grammar :contentReference[oaicite:4]{index=4}
const os        = require('os');

/* ── 1 · Tree-sitter bootstrap ───────────────────────────────────────────── */
const parser = new Parser();
parser.setLanguage(C);                                                 // grammar attach

/* ── 2 · CLI parsing (zero-dependency) ───────────────────────────────────── */
const argv        = process.argv.slice(2);
const srcDirs     = [];
let   testsDir    = 'tests';
let   crossDir    = process.env.CROSS_DIR || 'cross';
let   outFile     = 'repo_map.json';
const excludes    = [];
let   jobs        = 0;                                                 // 0 → auto

for (let i = 0; i < argv.length; ++i) {
  const arg  = argv[i];
  const need = () => {
    if (!argv[i + 1]) { console.error(`missing value for ${arg}`); process.exit(1); }
    return argv[++i];
  };

  switch (arg) {
    case '--src':    case '-s': srcDirs.push(need());                     break;
    case '--tests':  case '-t': testsDir  = need();                       break;
    case '--cross':  case '-c': crossDir  = need();                       break;
    case '--out':    case '-o': outFile   = need();                       break;
    case '--exclude':case '-x': excludes.push(need());                    break;
    case '--jobs':   case '-j': jobs = parseInt(need(), 10) || 0;         break;
    case '--help':   case '-h':
      console.log(`Usage: node repo_map.js [options]
  -s, --src DIR       Add source root (repeatable)     [default: src]
  -t, --tests DIR     Tests directory                  [default: tests]
  -c, --cross DIR     Meson cross-file directory       [default: cross]
  -o, --out FILE      Output JSON file                [default: repo_map.json]
  -x, --exclude GLOB  Ignore pattern (repeatable)
  -j, --jobs N        Parallel parses (0 = all CPUs)
  -h, --help          Show this help`);
      process.exit(0);
    default:
      console.error(`unknown option: ${arg}`); process.exit(1);
  }
}
if (srcDirs.length === 0) srcDirs.push('src');                           // default

/* ── 3 · Helpers ────────────────────────────────────────────────────────── */
const listCrossFiles = dir =>
  fs.existsSync(dir) ? fs.readdirSync(dir).filter(f => f.endsWith('.cross')) : [];

const sha256 = data =>
  crypto.createHash('sha256').update(data).digest('hex');                // hash :contentReference[oaicite:5]{index=5}

function functionsIn(source) {                                           // AST visitor
  const fns  = [];
  (function walk(node) {
    if (node.type === 'function_definition') {
      const idNode = node
        .childForFieldName('declarator')
        ?.descendantsOfType('identifier')[0];
      if (idNode) fns.push(idNode.text);
    }
    for (let i = 0; i < node.namedChildCount; ++i) walk(node.namedChild(i));
  })(parser.parse(source).rootNode);                                      // parse once
  return fns;
}

function parseFile(fp) {                                                 // per-file meta
  return { functions: functionsIn(fs.readFileSync(fp, 'utf8')) };
}

/* ── 4 · Concurrency setup ─────────────────────────────────────────────── */
const cpuCount = typeof os.availableParallelism === 'function'
  ? os.availableParallelism()                                            // Node ≥ 19 :contentReference[oaicite:6]{index=6}
  : os.cpus().length;                                                    // legacy :contentReference[oaicite:7]{index=7}

const limit    = pLimit(jobs > 0 ? jobs : cpuCount);                     // throttle

/* ── 5 · Main scan ═ async ═────────────────────────────────────────────── */
const files = Object.create(null);                                       // fp → meta

async function scan(patternRoot) {                                       // one root
  const patterns = [`${patternRoot}/**/*.c`];                            // C only
  const ignore   = ['**/build/**', '**/vendor/**', ...excludes];         // defaults + CLI
  const paths    = await fg(patterns, { ignore, onlyFiles: true, unique: true });  // :contentReference[oaicite:8]{index=8}

  await Promise.all(paths.map(fp =>
    limit(() => { files[fp] = parseFile(fp); })
  ));
}

(async () => {
  // 5.1 · gather sources ---------------------------------------------------
  for (const d of srcDirs)  await scan(d);
  await scan(testsDir);

  // 5.2 · assemble JSON ----------------------------------------------------
  const crossFiles = listCrossFiles(crossDir);
  const map = {
    generated_at : new Date().toISOString(),
    build_system : 'meson',
    cross_files  : crossFiles,
    toolchains   : crossFiles.map(f => f.replace('.cross', '')),
    src_roots    : srcDirs,
    tests_dir    : testsDir,
    test_suites  : fs.existsSync(testsDir)
                    ? fs.readdirSync(testsDir).filter(f => f.endsWith('.c'))
                    : [],
    files,
  };

  // 5.3 · write artefact ---------------------------------------------------
  const payload  = JSON.stringify(map, null, 2);
  const outPath  = path.resolve(outFile);
  fs.writeFileSync(outPath, payload);

  console.log(`repo_map written → ${outPath}`);
  console.log(`SHA-256 digest   : ${sha256(payload)}`);
})().catch(err => { console.error(err); process.exit(1); });
