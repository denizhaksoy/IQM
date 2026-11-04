// Continue an active scheduler, drain the declared cohort, then analyze it.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
async function main(){
 const [configPath,analysisPath]=process.argv.slice(2);
 if(!analysisPath||fs.existsSync(analysisPath))throw Error('Usage: scheduler-config.json fresh-analysis-root');
 const config=read(configPath),base=path.dirname(path.resolve(configPath)),out=path.resolve(base,config.output),cohort=path.resolve(base,config.cohort),lock=path.join(out,'scheduler.lock');
 const run=(file,args)=>new Promise((resolve,reject)=>{const c=cp.spawn(file,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error(file+' exited '+code)));});
 let waited=false,ticks=0;
 while(fs.existsSync(lock)){
  const owner=read(lock);if(!Number.isSafeInteger(owner.pid)||owner.pid<=0)throw Error('Invalid scheduler lock');
  try{process.kill(owner.pid,0);}catch(e){throw Error('Scheduler lock owner is unavailable; inspect prior attempt and lock manually: '+e.message);}
  if(ticks++%12===0)console.log('Waiting for active scheduler PID '+owner.pid);
  waited=true;await new Promise(resolve=>setTimeout(resolve,5000));
 }
 if(waited){
  const state=read(path.join(out,'checkpoint.json')),last=state.attempts.at(-1);
  if(last&&!state.videos.some(v=>v.video===last.video))throw Error('Previous scheduler did not accept its last attempt; inspect it before continuing');
 }
 await run(process.execPath,['tests/run_live_hdr_cohort.cjs',configPath,String(read(cohort).cases.length)]);
 await run(process.execPath,['tests/analyze_live_hdr_cohort.cjs',cohort,path.join(out,'video-predictions.json'),analysisPath,'--local-cohort']);
 await run('C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',['--quiet','tests/verify_live_hdr_cohort_analysis.m',path.join(analysisPath,'analysis.json'),path.join(analysisPath,'verification_octave.json')]);
 console.log('COMPLETE declared local cohort; original-author reproduction and full-GPU status remain separate.');
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
