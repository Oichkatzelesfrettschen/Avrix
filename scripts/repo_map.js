#!/usr/bin/env node
/*───────────────────────────────────────────────────────────────────────────────
  repo_map.js — harvest project metadata into repo_map.json
  ───────────────────────────────────────────────────────────────────────────────
  • Discovers C/C++ sources (default src/, tests/) using Tree-sitter.
  • Generates a JSON knowledge-graph consumed by meta-build agents.
  • CLI: node repo_map.js [--src DIR] [--tests DIR] [--cross DIR]
          [--output FILE]
     – defaults: src/, tests/, cross/, repo_map.json
  • Emits SHA-256 digest of the JSON on stdout for deterministic CI caching.
  • Requires:    npm i tree-sitter tree-sitter-c fast-glob p-limit
───────────────────────────────────────────────────────────────────────────────*/

'use strict';
const fs     = require('fs');
const path   = require('path'); // required before first use
const crypto = require('crypto');
const fg     = require('fast-glob');          // robust globbing, cross-platform
const pLimit = require('p-limit');            // concurrency throttle
const Parser = require('tree-sitter');
const C      = require('tree-sitter-c');

// ───────────────────────────── Tree-sitter setup ─────────────────────────────
const parser = new Parser();
parser.setLanguage(C);

// ───────────────────────────── CLI argument parse ────────────────────────────
const argv  = process.argv.slice(2);
const args  = { src: 'src', tests: 'tests', cross: 'cross', output: 'repo_map.json' };

for (let i = 0; i < argv.length; ++i) {
  const arg = argv[i];
  const need = () => {
    if (!argv[i + 1]) { console.error(`missing value for ${arg}`); process.exit(1); }
    return argv[++i];
  };
  switch (arg) {
    case '--src':    args.src    = need(); break;
    case '--tests':  args.tests  = need(); break;
    case '--cross':  args.cross  = need(); break;
    case '--output': args.output = need(); break;
    case '--help':
      console.log('Usage: node repo_map.js [--src DIR] [--tests DIR] [--cross DIR] [--output FILE]');
      process.exit(0);
    default:
      console.error(`unknown option: ${arg}`); process.exit(1);
  }
}

// ───────────────────────────── Helpers ───────────────────────────────────────
function listCrossFiles(dir) {
  try {
    return fs.readdirSync(dir).filter(f => f.endsWith('.cross'));
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
const limit   = pLimit(require('os').cpus().length);
const files   = {};                    // fp → {functions: [...]}

async function scan(root) {
  const patterns   = [`${root}/**/*.c`];
  const ignore     = ['**/build/**', '**/vendor/**'];
  const candidates = await fg(patterns, { ignore, onlyFiles: true, unique: true });

  await Promise.all(candidates.map(fp =>
    limit(async () => { files[fp] = parseFile(fp); })
  ));
}

(async () => {
  await scan(args.src);
  await scan(args.tests);

  // ───────────────────────────── Assemble repo map ───────────────────────────
  const crossFiles = listCrossFiles(args.cross);
  const map = {
    generated_at : new Date().toISOString(),
    build_system : 'meson',
    cross_files  : crossFiles,
    toolchains   : crossFiles.map(f => f.replace('.cross', '')),
    src_roots    : [args.src],
    tests_dir    : args.tests,
    test_suites  : fs.existsSync(args.tests)
                    ? fs.readdirSync(args.tests).filter(f => f.endsWith('.c'))
                    : [],
    files,
  };

  const json     = JSON.stringify(map, null, 2);
  const outPath  = path.resolve(args.output);
  fs.writeFileSync(outPath, json);

  // CI can key cache artefacts on this digest
  console.log(`repo_map written to ${outPath}`);
  console.log(`SHA-256: ${sha256(json)}`);
})().catch(err => { console.error(err); process.exit(1); });
