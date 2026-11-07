// Resume an interrupted local continuation without replacing historical evidence.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [rootArg,tag]=process.argv.slice(2);
 if(!rootArg||!tag||!/^[a-zA-Z0-9_-]+$/.test(tag))throw Error('Usage: existing-continuation-root unique-resume-tag');
 const root=path.resolve(rootArg),configPath=path.join(root,'config.json'),config=read(configPath),resolve=p=>path.resolve(root,p),results=resolve(config.output);
 const receipt=path.join(root,'resume_'+tag);fs.mkdirSync(receipt);
 const history=path.resolve('validation/live_hdr_cohort_scores_20261102_run01/results/checkpoint.json'),before=sha(history);
 if(before!==sha(path.join(root,'historical_checkpoint.json')))throw Error('Historical checkpoint changed');
 for(const issue of config.unresolved_reference_discrepancies||[]){if(sha(issue.original_score)!==issue.original_score_sha256)throw Error('Historical score changed');}
 const lock=path.join(results,'scheduler.lock');
 if(fs.existsSync(lock)){
  const owner=read(lock);if(!Number.isSafeInteger(owner.pid)||owner.pid<=0)throw Error('Invalid lock PID');
  let dead=false;try{process.kill(owner.pid,0);}catch(e){if(e.code==='ESRCH')dead=true;else throw e;}
  if(!dead)throw Error('Scheduler still alive; refusing duplicate');
  fs.renameSync(lock,path.join(receipt,'interrupted_scheduler.lock'));
 }
 const save=value=>fs.writeFileSync(path.join(receipt,'status.json'),JSON.stringify({time:new Date().toISOString(),...value},null,2));
 const run=(args)=>new Promise((resolve,reject)=>{const child=cp.spawn(process.execPath,args,{stdio:'inherit',windowsHide:true});child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error(args[0]+' exited '+code)));});
 try{
  save({stage:'evaluating_remaining_videos',historical_checkpoint_sha256:before});
  await run(['tests/run_live_hdr_cohort_pipeline.cjs',configPath,path.join(receipt,'analysis')]);
  const predictions=read(path.join(results,'video-predictions.json'));
  save({stage:'auditing_evidence'});
  await run(['tests/summarize_live_hdr_evidence.cjs',resolve(config.dataset),path.join(receipt,'evidence'),...predictions.videos.map(v=>path.resolve(results,v.root))]);
  const frozen=path.resolve('validation/live_hdr_public_backends_20261102_run01');
  save({stage:'verifying_public_backends'});
  await run(['tests/audit_live_hdr_public_backends.cjs',configPath,path.join(frozen,'cpu.exe'),path.join(frozen,'hybrid.exe'),path.join(frozen,'projection.spv'),path.join(receipt,'public_backends')]);
  if(sha(history)!==before)throw Error('Historical checkpoint modified');
  save({stage:'complete_local_validation_with_open_historical_discrepancy',videos:predictions.videos.length,historical_reference_discrepancy_resolved:false,original_paper_reproduction_verified:false,full_gpu_verified:false});
 }catch(e){save({stage:'failed',error:String(e.stack)});throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
