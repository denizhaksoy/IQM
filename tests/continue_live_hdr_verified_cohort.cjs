// Preserve the failed original cohort; continue only from an independently audited retry.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [originalConfigArg,diagnosticArg,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: original-config repeated-video-root fresh-continuation-root');
 const originalConfig=path.resolve(originalConfigArg),diagnostic=path.resolve(diagnosticArg),out=path.resolve(outArg),config=read(originalConfig),oldBase=path.dirname(originalConfig),resolve=p=>path.resolve(oldBase,p),oldResults=resolve(config.output),oldCheckpoint=path.join(oldResults,'checkpoint.json'),oldCheckpointHash=sha(oldCheckpoint),cohort=read(resolve(config.cohort));
 fs.mkdirSync(out,{recursive:true});const status=path.join(out,'continuation_status.json'),save=value=>{const temporary=status+'.tmp';fs.writeFileSync(temporary,JSON.stringify({time:new Date().toISOString(),...value},null,2));fs.renameSync(temporary,status);};
 const run=(exe,args)=>new Promise((resolve,reject)=>{const c=cp.spawn(exe,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error(exe+' exited '+code)));});
 try{
  save({stage:'waiting_for_repeated_video',diagnostic});let ticks=0;
  const marker=path.join(diagnostic,'repeated_video_acceptance.json');
  while(!fs.existsSync(marker)){
   if(fs.existsSync(path.join(diagnostic,'failure.txt')))throw Error('Repeated video failed; original cohort remains unchanged');
   const lock=path.join(diagnostic,'postprocess.lock');if(!fs.existsSync(lock))throw Error('Repeated-video postprocess is unavailable');
   process.kill(read(lock).pid,0);if(ticks++%12===0)console.log('Waiting for full repeated-video acceptance before continuation');
   await new Promise(resolve=>setTimeout(resolve,5000));
  }
  if(sha(oldCheckpoint)!==oldCheckpointHash)throw Error('Original cohort checkpoint changed while waiting');
  const verified=read(marker),plan=read(path.join(diagnostic,'plan.json'));
  if(!verified.exact||!verified.independent_aggregation_verified||verified.reference_runs_per_frame<3||plan.reference_runs<3)throw Error('Repeated video not independently verified');
  const c=cohort.cases.find(c=>c.video===plan.test_id&&c.reference===plan.reference_id);if(!c)throw Error('Repeated video outside cohort');accepted(diagnostic,c,resolve(config.dataset));
  const old=read(oldCheckpoint);if(old.videos.some(v=>v.video===c.video))throw Error('Diagnostic would overwrite an accepted original video');
  const context=read(path.join(diagnostic,'investigation.json')),originalScore=path.resolve(context.historical_frame,'official/score.json');
  if(!context.historical_original_discrepancy_unresolved||!fs.existsSync(originalScore))throw Error('Historical discrepancy provenance missing');
  fs.copyFileSync(originalScore,path.join(out,'historical_original_score.json'));fs.copyFileSync(oldCheckpoint,path.join(out,'historical_checkpoint.json'));
  const discrepancy={video:c.video,zero_based_frame:context.priority_zero_based_frame,status:'historical_reference_discrepancy_unresolved',original_score:originalScore,original_score_sha256:sha(originalScore),new_repeated_video:diagnostic,original_score_preserved:true};
  const relative=p=>path.relative(out,p);
  const fresh={dataset:relative(resolve(config.dataset)),official:relative(resolve(config.official)),bundle:relative(resolve(config.bundle)),cohort:relative(resolve(config.cohort)),output:'results',reference_runs:3,accepted:old.videos.map(v=>({video:v.video,root:relative(path.resolve(oldResults,v.root))})).concat([{video:c.video,root:relative(diagnostic)}]),unresolved_reference_discrepancies:[discrepancy]};
  const freshConfig=path.join(out,'config.json');fs.writeFileSync(freshConfig,JSON.stringify(fresh,null,2));
  save({stage:'evaluating_remaining_videos',imported_original_videos:old.videos.length,verified_retry:c.video,historical_reference_discrepancy_resolved:false});
  await run(process.execPath,['tests/run_live_hdr_cohort_pipeline.cjs',freshConfig,path.join(out,'analysis')]);
  const predictions=read(path.join(out,'results/video-predictions.json')),roots=predictions.videos.map(v=>path.resolve(out,'results',v.root));
  await run(process.execPath,['tests/summarize_live_hdr_evidence.cjs',resolve(config.dataset),path.join(out,'evidence'),...roots]);
  if(sha(oldCheckpoint)!==oldCheckpointHash)throw Error('Original cohort checkpoint was modified');
  save({stage:'verifying_public_backends',videos:predictions.videos.length,historical_reference_discrepancy_resolved:false});
  const frozen=path.resolve('validation/live_hdr_public_backends_20261102_run01');
  await run(process.execPath,['tests/audit_live_hdr_public_backends.cjs',freshConfig,path.join(frozen,'cpu.exe'),path.join(frozen,'hybrid.exe'),path.join(frozen,'projection.spv'),path.join(out,'public_backends')]);
  save({stage:'complete_local_validation_with_open_historical_discrepancy',videos:predictions.videos.length,reference_runs_for_new_videos:3,imported_original_videos:old.videos.length,historical_reference_discrepancy_resolved:false,original_paper_reproduction_verified:false,full_gpu_verified:false});
 }catch(e){save({stage:'failed',error:String(e.stack)});throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
