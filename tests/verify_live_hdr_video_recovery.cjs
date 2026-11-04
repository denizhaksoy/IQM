// End-to-end fresh-decode recovery rehearsal; preserves and never interrupts seed.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
async function main(){
 const [configFile,seedRoot,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: scheduler-config seed-video-root fresh-recovery-root');
 const config=read(configFile),base=path.dirname(path.resolve(configFile)),resolve=p=>path.resolve(base,p);
 let ticks=0;
 while(!fs.existsSync(path.join(seedRoot,'aggregation_octave.json'))){
  if(fs.existsSync(path.join(seedRoot,'failure.txt')))throw Error('Seed evaluation failed; recovery test stops');
  if(ticks++%12===0)console.log('Waiting for independent aggregation of seed video');
  await new Promise(resolve=>setTimeout(resolve,5000));
 }
 const plan=read(path.join(seedRoot,'plan.json'));if(!plan.calculation_sha256)throw Error('Seed predates calculation fingerprint; cannot test exact-frame reuse');
 const run=(exe,args)=>new Promise((resolve,reject)=>{const c=cp.spawn(exe,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error('Recovery command exited '+code)));});
 await run(process.execPath,['tests/run_live_hdr_video_pair.cjs',resolve(config.dataset),resolve(config.official),resolve(config.bundle),out,plan.test_id,plan.reference_id,path.resolve(seedRoot)]);
 for(const index of plan.indices){const r=read(path.join(out,'frame'+String(index).padStart(4,'0'),'comparison.json'));if(!r.exact||!r.reused||!r.reuse_source)throw Error('Not every frame recovered from verified seed');}
 await run(process.execPath,['tests/summarize_live_hdr_video.cjs',out,resolve(config.dataset)]);
 await run('C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',['--quiet','tests/verify_live_hdr_video_aggregation.m',out]);
 const prior=read(path.join(seedRoot,'aggregation_octave.json')),actual=read(path.join(out,'aggregation_octave.json'));
 if(prior.Q_JOD_hex!==actual.Q_JOD_hex||prior.frames!==actual.frames)throw Error('Recovered video aggregate differs');
 const result={scope:'recovery rehearsal on repeated video, not additional unique dataset cases; seed was not interrupted',exact:true,all_frames_freshly_decoded_and_reused:true,frames:actual.frames,Q_JOD_hex:actual.Q_JOD_hex,seed:path.resolve(seedRoot)};
 fs.writeFileSync(path.join(out,'recovery_acceptance.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
