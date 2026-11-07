// New-backend validation of a separately accepted video, including legacy plans
// whose identifiers are verified from saved metadata and hashed MP4 paths.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [videoArg,runtimeArg,bundleArg,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: independently-accepted-video-root frozen-mutual-runtime bundle fresh-out');
 const source=path.resolve(videoArg),sourceRuntime=path.resolve(runtimeArg),bundle=path.resolve(bundleArg),out=path.resolve(outArg),plan=read(path.join(source,'plan.json')),acceptance=read(path.join(source,'acceptance.json')),oldScore=read(path.join(source,'video_score.json')),aggregate=read(path.join(source,'aggregation_octave.json'));
 const video=acceptance.metadata.video,reference=acceptance.metadata.ref;
 if(path.basename(plan.test,'.mp4')!==video||path.basename(plan.reference,'.mp4')!==reference||sha(plan.test)!==plan.test_sha256||sha(plan.reference)!==plan.reference_sha256)throw Error('Video identity/bytes differ');
 if(!acceptance.exact_all_selected_frames||!oldScore.complete||!oldScore.exact||!aggregate.exact||aggregate.class!=='single'||aggregate.frames!==oldScore.frames||oldScore.frames!==plan.indices.length||plan.indices.length!==Math.ceil(plan.decoded_frames/30))throw Error('Prior acceptance incomplete');
 if(plan.decoded_frames!==Number(acceptance.metadata.frm_cnt)||plan.decoded_frames!==Number(acceptance.metadata.ref_frm_cnt)||plan.zero_based_phase!==0||plan.stride!==30||plan.png_pixel_format!=='rgb48be'||plan.ppd!==62.729130063099447)throw Error('Unsupported input protocol');
 if(plan.reference_timestamps.length!==plan.decoded_frames||plan.test_timestamps.length!==plan.decoded_frames||plan.reference_timestamps.some((t,i)=>typeof t!=='string'||!Number.isFinite(Number(t))||t!==plan.test_timestamps[i]))throw Error('Decoded alignment differs');
 const manifest=read(path.join(bundle,'files.json')),dll='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll';if(sha(dll)!==manifest.external_dependency.sha256||sha(dll)!==plan.fftw_sha256)throw Error('FFTW differs');
 fs.mkdirSync(out);const runtime=path.join(out,'runtime');fs.mkdirSync(runtime);fs.mkdirSync(path.join(runtime,'model'));
 const runtimeFiles=['experimental.exe','projection.spv','mask.spv','mutual.spv'];for(const file of runtimeFiles)fs.copyFileSync(path.join(sourceRuntime,file),path.join(runtime,file));
 for(const e of manifest.entries.filter(e=>e.file.replace(/\\/g,'/').startsWith('data/'))){const file=path.join(sourceRuntime,'model',path.basename(e.file));if(sha(file)!==e.sha256)throw Error('Model differs');fs.copyFileSync(file,path.join(runtime,'model',path.basename(e.file)));}
 const sourceFiles=['plan.json','acceptance.json','video_score.json','aggregation_octave.json'].map(file=>({file,sha256:sha(path.join(source,file))}));
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify({scope:'additional accepted real video; new hybrid backend, not original paper or full GPU',video,reference,source,sourceFiles,runtime:runtimeFiles.map(file=>({file,sha256:sha(path.join(runtime,file))}))},null,2));
 const jobs=plan.indices.map((index,k)=>{if(index!==k*30)throw Error('Sampling differs');const frame=path.join(source,'frame'+String(index).padStart(4,'0')),comparison=path.join(frame,'comparison.json'),record=read(comparison);if(!record.exact||record.zero_based_frame!==index||record.timestamp!==plan.reference_timestamps[index]||record.official.Q_hex!==record.cpp.Q_hex||record.official.Q_JOD_hex!==record.cpp.Q_JOD_hex)throw Error('Old frame acceptance differs');return {index,frame,record,comparison,comparison_sha256:sha(comparison)};});
 const results=[];let next=0,failed=false;
 const save=(stage,error)=>{const temp=path.join(out,'checkpoint.tmp');fs.writeFileSync(temp,JSON.stringify({time:new Date().toISOString(),scope:'one additional real video with frozen matrices+comparison+mutual backend',video,reference,stage,complete:stage==='complete',total:jobs.length,completed:results.length,results,error,full_gpu_verified:false,original_paper_reproduction_verified:false},null,2));fs.renameSync(temp,path.join(out,'checkpoint.json'));};save('running');
 const run=args=>new Promise((resolve,reject)=>{const child=cp.spawn(process.execPath,args,{stdio:'inherit',windowsHide:true});child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error('Frame wrapper exited '+code)));});
 const worker=async()=>{try{while(!failed&&next<jobs.length){const job=jobs[next++],root=path.join(out,'frame'+String(job.index).padStart(4,'0'));fs.mkdirSync(root);const stock=path.join(root,'official_score.json');fs.writeFileSync(stock,JSON.stringify(job.record.official));
  await run(['tests/run_live_hdr_mutual_experimental.cjs',path.join(runtime,'experimental.exe'),path.join(job.frame,'decoded/reference.png'),path.join(job.frame,'decoded/test.png'),path.join(runtime,'model'),dll,path.join(runtime,'projection.spv'),path.join(runtime,'mask.spv'),path.join(runtime,'mutual.spv'),stock,path.join(root,'evaluation')]);
  if(sha(job.comparison)!==job.comparison_sha256)throw Error('Original frame record changed');const c=read(path.join(root,'evaluation/comparison.json'));if(!c.exact||!c.backendVerified||c.validationErrors)throw Error('New backend frame not accepted');results.push({index:job.index,Q_hex:c.Q_hex,Q_JOD_hex:c.Q_JOD_hex,exact:true});save('running');console.log('ADDITIONAL VIDEO '+results.length+'/'+jobs.length+' EXACT');
 } }catch(e){failed=true;throw e;}};
 try{
  const workers=await Promise.allSettled([worker(),worker()]);const bad=workers.find(r=>r.status==='rejected');if(bad)throw bad.reason;if(results.length!==jobs.length)throw Error('Incomplete video');
  let sum=0;const word=Buffer.alloc(4);for(const r of [...results].sort((a,b)=>a.index-b.index)){word.writeUInt32LE(parseInt(r.Q_JOD_hex,16));sum=Math.fround(sum+word.readFloatLE());}const mean=Math.fround(sum/results.length);word.writeFloatLE(mean);const meanHex=word.readUInt32LE().toString(16).padStart(8,'0');
  if(meanHex!==aggregate.Q_JOD_hex||mean!==oldScore.official_Q_JOD)throw Error('Native Single mean differs');
  for(const e of sourceFiles)if(sha(path.join(source,e.file))!==e.sha256)throw Error('Original evidence changed');
  fs.writeFileSync(path.join(out,'video_mean.json'),JSON.stringify({exact:true,frames:results.length,Q_JOD:mean,Q_JOD_hex:meanHex,independent_original_octave_aggregation_verified:true},null,2));save('complete');
 }catch(e){save('failed',String(e.stack));throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
