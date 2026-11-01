// Decode embedded text as data only; never eval downloaded JavaScript.
const fs=require('node:fs'),path=require('node:path');
const [input,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: saved-html fresh-root');fs.mkdirSync(out,{recursive:true});
const text=fs.readFileSync(input,'utf8');let index=0;const links=[];
for(const match of text.matchAll(/const content = '([^']*)';/g)){
 const html=match[1].replace(/\\u([0-9a-f]{4})/gi,(_,s)=>String.fromCharCode(parseInt(s,16)));
 for(const link of html.matchAll(/href=["']([^"']+)["']/g))links.push(link[1].replace(/&amp;/g,'&'));
 fs.writeFileSync(path.join(out,`block${index++}.html`),html);
}
fs.writeFileSync(path.join(out,'links.json'),JSON.stringify([...new Set(links)],null,2));
console.log([...new Set(links)].join('\n'));
