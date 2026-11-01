const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [root]=process.argv.slice(2);if(!root)throw Error('Usage: accepted-demo-root');
const output=path.join(root,'acceptance.json');if(fs.existsSync(output))throw Error('Preserve prior acceptance summary');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'files.json'),'utf8'));
for(const e of manifest.entries){const hash=crypto.createHash('sha256').update(fs.readFileSync(path.join(root,e.file))).digest('hex');if(hash!==e.sha256)throw Error('Bundle mutated: '+e.file);}
const cases=fs.readdirSync(root).filter(n=>n.startsWith('results_')).map(n=>({directory:n,...JSON.parse(fs.readFileSync(path.join(root,n,'comparison.json'),'utf8'))}));
if(cases.length!==2||cases.some(c=>!c.exact)||!cases.some(c=>c.directory.startsWith('results_frame0000_'))||!cases.some(c=>c.directory.startsWith('results_frame0030_')))throw Error('Two-frame exact acceptance incomplete');
fs.writeFileSync(output,JSON.stringify({scope:'two full-resolution Single CPU frame pairs only; not full video/dataset',cases},null,2)+'\n');
for(const c of cases)console.log(c.directory+' exact='+c.exact);
