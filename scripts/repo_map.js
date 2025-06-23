// -----------------------------------------------------------------------------
// repo_map.js — gather source metadata in repo_map.json
// -----------------------------------------------------------------------------
const Parser = require('tree-sitter');
const fs = require('fs');
const path = require('path');

const TREE_SITTER_C_PATH =
  process.env.TREE_SITTER_C_PATH || path.resolve(__dirname, 'tree-sitter-c');
const C = require(TREE_SITTER_C_PATH);

const parser = new Parser();
parser.setLanguage(C);

function parseFile(file) {
  const source = fs.readFileSync(file, 'utf8');
  const tree = parser.parse(source);
  const functions = [];
  function walk(node) {
    if (node.type === 'function_definition') {
      const declarator = node.childForFieldName('declarator');
      const id = declarator && declarator.descendantsOfType('identifier')[0];
      if (id) functions.push(id.text);
    }
    for (let i = 0; i < node.namedChildCount; i++) {
      walk(node.namedChild(i));
    }
  }
  walk(tree.rootNode);
  return functions;
}

const files = {};
function scan(dir) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const fp = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      scan(fp);
    } else if (fp.endsWith('.c')) {
      files[fp] = { functions: parseFile(fp) };
    }
  }
}

// ────────────────────────────── CLI ──────────────────────────────────
// Usage: node repo_map.js [--src dir]... [--tests dir] [--out file]
// Multiple --src options may be given. Defaults are src/, tests/, repo_map.json
const argv = process.argv.slice(2);
const srcDirs = [];
let testsDir = 'tests';
let outFile = 'repo_map.json';

for (let i = 0; i < argv.length; i++) {
  const arg = argv[i];
  switch (arg) {
    case '--src':
    case '-s':
      if (!argv[i + 1]) {
        console.error('missing argument for --src');
        process.exit(1);
      }
      srcDirs.push(argv[++i]);
      break;
    case '--tests':
    case '-t':
      if (!argv[i + 1]) {
        console.error('missing argument for --tests');
        process.exit(1);
      }
      testsDir = argv[++i];
      break;
    case '--out':
    case '-o':
      if (!argv[i + 1]) {
        console.error('missing argument for --out');
        process.exit(1);
      }
      outFile = argv[++i];
      break;
    default:
      console.error(`unknown option: ${arg}`);
      process.exit(1);
  }
}

if (srcDirs.length === 0) {
  srcDirs.push('src');
}

for (const dir of srcDirs) {
  scan(dir);
}
scan(testsDir);

const map = {
  files,
  build_system: 'meson',
  cross_files: fs.readdirSync('cross').filter(f => f.endsWith('.cross')),
  toolchains: fs.readdirSync('cross')
    .filter(f => f.endsWith('.cross'))
    .map(f => f.replace('.cross', '')),
  test_suites: fs.readdirSync(testsDir).filter(f => f.endsWith('.c')),
};

fs.writeFileSync(outFile, JSON.stringify(map, null, 2));
