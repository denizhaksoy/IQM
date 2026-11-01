const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const [exe,root]=process.argv.slice(2);if(!root||fs.existsSync(root))throw Error('Usage: CPU-executable fresh-root');fs.mkdirSync(root,{recursive:true});
const base=['--input','unused.png','--ref','unused.png','--encoding','pq-bt2020-rgb16','--ppd','62.729130063099447','--model-data','unused','--fftw','unused.dll'];
const results=[];function run(name,args,success){const r=cp.spawnSync(exe,args,{encoding:'utf8',windowsHide:true});if(r.error)throw r.error;const passed=success?r.status===0:r.status!==0;if(!passed)throw Error(name+' unexpected exit');results.push({name,passed,exit:r.status,stdout:r.stdout,stderr:r.stderr});}
run('help',['--help'],true);
for(const [name,key,value] of [['ambiguous-encoding','--encoding','srgb'],['nonfinite-ppd','--ppd','NaN'],['trailing-ppd-text','--ppd','62junk']]){const args=base.slice();args[args.indexOf(key)+1]=value;const out=path.join(root,name);run(name,[...args,'--output',out],false);if(fs.existsSync(out))throw Error(name+' created output before validation');}
run('missing-required-encoding',base.filter((_,i)=>i!==4&&i!==5).concat(['--output',path.join(root,'missing')]),false);
run('duplicate-ppd',[...base,'--ppd','60','--output',path.join(root,'duplicate')],false);
const old=path.join(root,'existing');fs.mkdirSync(old);fs.writeFileSync(path.join(old,'sentinel.txt'),'preserve');run('existing-output',[...base,'--output',old],false);if(fs.readFileSync(path.join(old,'sentinel.txt'),'utf8')!=='preserve')throw Error('Existing output modified');
fs.writeFileSync(path.join(root,'checks.json'),JSON.stringify({passed:true,cases:results},null,2));console.log(results.length+' CLI checks passed');
