// Postprocess a diagnostic repeated-reference run; never merge it into a cohort.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
async function main(){
 const [root,dataset,pidArg]=process.argv.slice(2),pid=Number(pidArg);
 if(!dataset||!Number.isSafeInteger(pid)||pid<=0)throw Error('Usage: repeated-video-root dataset runner-PID');
 const marker=path.join(root,'repeated_video_acceptance.json'),lock=path.join(root,'postprocess.lock');
 if(fs.existsSync(marker)||fs.existsSync(path.join(root,'acceptance.json')))throw Error('Preserve previous postprocessing');
 const fd=fs.openSync(lock,'wx');fs.writeFileSync(fd,JSON.stringify({pid:process.pid}));fs.closeSync(fd);
 try{
  let ticks=0;while(!fs.existsSync(path.join(root,'video_score.json'))){
   if(fs.existsSync(path.join(root,'failure.txt')))throw Error('Repeated-reference video failed; no acceptance');
   process.kill(pid,0);if(ticks++%12===0)console.log('Waiting for complete repeated-reference diagnostic');
   await new Promise(resolve=>setTimeout(resolve,5000));
  }
  const plan=read(path.join(root,'plan.json'));if(plan.reference_runs<3)throw Error('Three or more reference runs required');
  const run=(exe,args)=>new Promise((resolve,reject)=>{const c=cp.spawn(exe,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error('Postprocessing exited '+code)));});
  await run(process.execPath,['tests/summarize_live_hdr_video.cjs',root,dataset]);
  await run('C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',['--quiet','tests/verify_live_hdr_video_aggregation.m',root]);
  const a=read(path.join(root,'acceptance.json'));accepted(root,{video:plan.test_id,reference:plan.reference_id,metadata_frames:Number(a.metadata.frm_cnt)},dataset);
  const result={scope:'fresh repeated-reference diagnostic only; does not resolve prior discrepant records or promote a cohort',exact:true,frames:plan.indices.length,reference_runs_per_frame:plan.reference_runs,independent_aggregation_verified:true,cohort_updated:false};
  fs.writeFileSync(marker,JSON.stringify(result,null,2));console.log(JSON.stringify(result));
 }finally{fs.unlinkSync(lock);}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
