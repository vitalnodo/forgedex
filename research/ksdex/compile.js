const KaitaiStructCompiler = require('kaitai-struct-compiler');
const yaml = require('js-yaml'), fs = require('fs'), path = require('path');
const root = process.argv[2], ksy = process.argv[3], out = process.argv[4];
const importer = { importYaml: (name, mode) => {
  const p = (mode === 'abs' || name.startsWith('/')) ? path.join(root, name + '.ksy') : path.join(path.dirname(ksy), name + '.ksy');
  return Promise.resolve(yaml.load(fs.readFileSync(p, 'utf8')));
}};
const c = (typeof KaitaiStructCompiler === "function") ? new KaitaiStructCompiler() : KaitaiStructCompiler;
c.compile('python', yaml.load(fs.readFileSync(ksy, 'utf8')), importer, false).then(files => {
  for (const [n, s] of Object.entries(files)) { fs.writeFileSync(path.join(out, n), s); console.log('wrote', n); }
}).catch(e => { console.error('ERR', e && (e.stack || e)); process.exit(1); });
