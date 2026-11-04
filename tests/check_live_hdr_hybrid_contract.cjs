const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const [cpu,hybrid,png,data,dll,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: CPU-exe hybrid-exe rgb16.png data fftw-dll fresh-root');fs.mkdirSync(out,{recursive:true});
const base=['--input',png,'--ref',png,'--encoding','pq-bt2020-rgb16','--ppd','62.729130063099447','--model-data',data,'--fftw',dll];const cases=[];
function reject(name,exe,args,folder){const r=cp.spawnSync(exe,args,{encoding:'utf8',windowsHide:true,maxBuffer:8*1024*1024});if(r.error)throw r.error;if(r.status===0||fs.existsSync(path.join(folder,'score.json')))throw Error(name+' silently succeeded/fell back');cases.push({name,exit:r.status,stderr:r.stderr,stdout:r.stdout});}
const missing=path.join(out,'missing');reject('hybrid-requires-shader',hybrid,[...base,'--output',missing],missing);if(fs.existsSync(missing))throw Error('Missing shader initialized output');
const cpuOut=path.join(out,'cpu');reject('CPU-rejects-GPU-shader-argument',cpu,[...base,'--projection-shader','missing.spv','--output',cpuOut],cpuOut);if(fs.existsSync(cpuOut))throw Error('CPU accepted GPU option');
const failed=path.join(out,'bad-shader');reject('bad-GPU-shader-no-CPU-fallback',hybrid,[...base,'--projection-shader',path.join(out,'missing.spv'),'--output',failed],failed);
fs.writeFileSync(path.join(out,'checks.json'),JSON.stringify({scope:'CLI/provider contract checks, no numerical dataset case added',passed:true,cases},null,2));console.log('3 hybrid/backend contract checks passed; no silent CPU fallback');
