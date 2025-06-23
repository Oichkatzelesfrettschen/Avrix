const Parser = require('tree-sitter');
const C = require('/tmp/tree-sitter-c');
const fs = require('fs');
const path = require('path');

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
    if (entry.isDirectory()) scan(fp);
    else if (fp.endsWith('.c')) {
      files[fp] = { functions: parseFile(fp) };
    }
  }
}

scan('src');
scan('tests');

const map = {
  files,
  build_system: 'meson',
  cross_files: fs.readdirSync('cross').filter(f => f.endsWith('.cross')),
  toolchains: fs.readdirSync('cross').filter(f => f.endsWith('.cross')).map(f => f.replace('.cross','')),
  test_suites: fs.readdirSync('tests').filter(f => f.endsWith('.c')),
};

fs.writeFileSync('repo_map.json', JSON.stringify(map, null, 2));
