// Sequential, restartable video scheduler. Every numerical attempt gets a fresh root.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {referenceConsistency}=require('./live_hdr_reference_consistency.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe';
function accepted(root,c,dataset){
 const plan=read(path.join(root,'plan.json')),s=read(path.join(root,'video_score.json')),a=read(path.join(root,'acceptance.json')),o=read(path.join(root,'aggregation_octave.json'));
 if(plan.test_id!==c.video||plan.reference_id!==c.reference||!s.complete||!s.exact||!a.exact_all_selected_frames||!o.exact||o.class!=='single'||s.frames!==Math.ceil(c.metadata_frames/30)||o.frames!==s.frames||s.official_Q_JOD!==s.cpp_Q_JOD)throw Error('Incomplete acceptance: '+c.video);
 if(plan.zero_based_phase!==0||plan.stride!==30||plan.ppd!==62.729130063099447||plan.png_pixel_format!=='rgb48be'||plan.metadata_sha256!==sha(path.join(dataset,'metadata/video_info_with_bitrate.csv')))throw Error('Protocol changed');
 for(const [id,key] of [[c.video,'test_sha256'],[c.reference,'reference_sha256']])if(sha(path.join(dataset,'sample_pair',id+'.mp4'))!==plan[key])throw Error('Video bytes changed');
 const b=Buffer.alloc(4);b.writeFloatLE(s.cpp_Q_JOD);if(o.Q_JOD_hex!==b.readUInt32LE().toString(16).padStart(8,'0'))throw Error('Aggregation changed');
 if(plan.decoded_frames!==c.metadata_frames||plan.indices.length!==s.frames||JSON.stringify(s.indices)!==JSON.stringify(plan.indices))throw Error('Sample count changed');
 if(plan.reference_timestamps.length!==plan.decoded_frames||plan.test_timestamps.length!==plan.decoded_frames||plan.reference_timestamps.some((t,i)=>typeof t!=='string'||!Number.isFinite(Number(t))||t!==plan.test_timestamps[i]))throw Error('Decoded alignment changed');
 let sum=0;
 for(let k=0;k<plan.indices.length;k++){
  const i=plan.indices[k];if(i!==k*30)throw Error('Sampling changed');
  const frame=path.join(root,'frame'+String(i).padStart(4,'0')),r=read(path.join(frame,'comparison.json'));
  if(!r.exact||r.zero_based_frame!==i||r.official.Q_hex!==r.cpp.Q_hex||r.official.Q_JOD_hex!==r.cpp.Q_JOD_hex)throw Error('Frame result changed');
  if((plan.reference_runs??1)>1){
   const audit=read(path.join(frame,'reference_consistency.json'));
   if(audit.files.length!==plan.reference_runs)throw Error('Reference repeat count changed');
   const records=audit.files.map(e=>{const file=path.join(frame,e.file);if(sha(file)!==e.sha256)throw Error('Reference repeat bytes changed');return read(file);});
   const verified=referenceConsistency(records);
   if(!verified.all_identical||verified.Q_hex!==r.official.Q_hex||verified.Q_JOD_hex!==r.official.Q_JOD_hex||audit.observations!==verified.observations)throw Error('Reference repetitions differ');
  }
  if(r.timestamp!==plan.reference_timestamps[i]||r.official.inputs[0].source_sha256!==r.reference_png_sha256||r.official.inputs[1].source_sha256!==r.test_png_sha256)throw Error('Frame provenance changed');
  if(!/^[0-9a-f]{8}$/i.test(r.cpp.Q_JOD_hex))throw Error('Invalid Single score');
  const word=Buffer.alloc(4);word.writeUInt32LE(parseInt(r.cpp.Q_JOD_hex,16));const value=word.readFloatLE();if(!Number.isFinite(value))throw Error('Nonfinite frame score');sum=Math.fround(sum+value);
  for(const [file,key] of [['reference.png','reference_png_sha256'],['test.png','test_png_sha256']])if(sha(path.join(frame,'decoded',file))!==r[key])throw Error('Decoded input changed');
 }
 if(Math.fround(sum/s.frames)!==s.cpp_Q_JOD)throw Error('Frame-to-video mean changed');
}
function run(exe,args){return new Promise((resolve,reject)=>{const child=cp.spawn(exe,args,{stdio:'inherit',windowsHide:true});child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error(exe+' exited '+code)));});}
async function main(){
 const [configPath,limitText='1']=process.argv.slice(2),limit=Number(limitText);
 if(!configPath||!Number.isSafeInteger(limit)||limit<1)throw Error('Usage: scheduler-config.json maximum-new-videos (default 1)');
 const config=read(configPath),base=path.dirname(path.resolve(configPath)),resolve=p=>path.resolve(base,p);
 const referenceRuns=config.reference_runs??1;
 if(!Number.isSafeInteger(referenceRuns)||referenceRuns<1||referenceRuns>5)throw Error('Invalid configured reference repeat count');
 const dataset=resolve(config.dataset),official=resolve(config.official),bundle=resolve(config.bundle),out=resolve(config.output),cohortPath=resolve(config.cohort),cohort=read(cohortPath);
 fs.mkdirSync(out,{recursive:true});
 const fingerprint=sha(cohortPath),lock=path.join(out,'scheduler.lock');
 // A stale lock requires inspecting the recorded PID; never silently run two schedulers.
 const fd=fs.openSync(lock,'wx');fs.writeFileSync(fd,JSON.stringify({pid:process.pid,started:new Date().toISOString()}));fs.closeSync(fd);
 try{
  const statePath=path.join(out,'checkpoint.json');let state=fs.existsSync(statePath)?read(statePath):{cohort_sha256:fingerprint,videos:[],attempts:[]};
  if(state.cohort_sha256!==fingerprint)throw Error('Cohort changed since checkpoint');
  if(new Set(cohort.cases.map(c=>c.video)).size!==cohort.cases.length||new Set(state.videos.map(v=>v.video)).size!==state.videos.length)throw Error('Duplicate cohort/checkpoint video');
  const atomic=(file,value)=>{const temporary=file+'.'+process.pid+'.tmp';fs.writeFileSync(temporary,JSON.stringify(value,null,2));fs.renameSync(temporary,file);};
  const save=()=>{atomic(statePath,state);atomic(path.join(out,'video-predictions.json'),{format:'IQM_LIVE_HDR_VIDEO_PREDICTIONS_1',videos:state.videos,unresolved_reference_discrepancies:config.unresolved_reference_discrepancies??[]});};
  for(const seed of config.accepted||[]){const c=cohort.cases.find(c=>c.video===seed.video);if(!c)throw Error('Seed outside cohort');const root=resolve(seed.root);accepted(root,c,dataset);if(!state.videos.some(v=>v.video===c.video))state.videos.push({video:c.video,root:path.relative(out,root)});}
  for(const v of state.videos){const c=cohort.cases.find(c=>c.video===v.video);if(!c)throw Error('Checkpoint outside cohort');accepted(path.resolve(out,v.root),c,dataset);}
  save();let started=0;
  for(const c of cohort.cases){if(state.videos.some(v=>v.video===c.video))continue;if(started++>=limit)break;
   const previous=state.attempts.findLast(a=>a.video===c.video),seed=previous?path.resolve(out,previous.root):null;
   const root=path.join(out,c.video+'_'+Date.now());state.attempts.push({video:c.video,root:path.relative(out,root),started:new Date().toISOString(),seed_root:seed});save();
   await run(process.execPath,['tests/run_live_hdr_video_pair.cjs',dataset,official,bundle,root,c.video,c.reference,seed??'-',String(referenceRuns)]);
   await run(process.execPath,['tests/summarize_live_hdr_video.cjs',root,dataset]);
   await run(octave,['--quiet','tests/verify_live_hdr_video_aggregation.m',root]);
   accepted(root,c,dataset);state.videos.push({video:c.video,root:path.relative(out,root)});save();console.log('COHORT accepted '+state.videos.length+'/'+cohort.cases.length);
  }
  console.log(JSON.stringify({accepted:state.videos.length,total:cohort.cases.length,complete:state.videos.length===cohort.cases.length,predictions:path.join(out,'video-predictions.json')}));
 }finally{fs.unlinkSync(lock);}
}
module.exports={accepted};
if(require.main===module)main().catch(e=>{console.error(e.stack);process.exitCode=1;});
