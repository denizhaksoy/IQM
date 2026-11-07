// After representative regression, validate the frozen new backend on EVERY
// accepted sampled frame. No new official scores or undocumented settings.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [configArg,representativeArg,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: accepted-cohort-config active-mutual-representative-root fresh-full-frame-root');
 const configPath=path.resolve(configArg),base=path.dirname(configPath),config=read(configPath),resolve=p=>path.resolve(base,p),representative=path.resolve(representativeArg),out=path.resolve(outArg),source=resolve(config.output),cohortPath=resolve(config.cohort),cohort=read(cohortPath),dataset=resolve(config.dataset);
 fs.mkdirSync(out);const log=message=>fs.appendFileSync(path.join(out,'progress.log'),new Date().toISOString()+' '+message+'\n');
 const results=[];let total=0;
 const save=(stage,extra={})=>{const temp=path.join(out,'checkpoint.tmp');fs.writeFileSync(temp,JSON.stringify({time:new Date().toISOString(),stage,complete:stage==='complete',scope:'all accepted phase0/stride30 frame pairs, frozen experimental matrices+comparison+mutual backend; not original paper or full GPU',total_frames:total,completed_frames:results.length,historical_reference_discrepancy_resolved:false,original_paper_reproduction_verified:false,full_gpu_verified:false,...extra},null,2));fs.renameSync(temp,path.join(out,'checkpoint.json'));};
 try{
  save('waiting_for_representative_regression');const start=Date.now();
  while(true){
   const checkpoint=read(path.join(representative,'checkpoint.json'));
   if(checkpoint.stage==='failed')throw Error('Representative regression failed; full-frame sweep not started');
   if(checkpoint.complete){if(checkpoint.results.length!==cohort.cases.length||checkpoint.results.some(r=>!r.exact||r.gpu_mutual_dispatches!==6))throw Error('Representative regression incomplete');break;}
   if(Date.now()-start>24*3600*1000)throw Error('Representative regression wait exceeded 24h; inspect its process');
   await new Promise(resolve=>setTimeout(resolve,5000));
  }
  const provenance=read(path.join(representative,'provenance.json'));
  const frozen=[['experimental.exe','executable_sha256'],['projection.spv','projection_sha256'],['mask.spv','mask_sha256'],['mutual.spv','mutual_sha256']];
  const runtime=path.join(out,'runtime');fs.mkdirSync(runtime);
  for(const [file,key] of frozen){const original=path.join(representative,file);if(sha(original)!==provenance[key])throw Error('Representative runtime changed');fs.copyFileSync(original,path.join(runtime,file));}
  const data=path.join(runtime,'model');fs.mkdirSync(data);const bundle=resolve(config.bundle),bundleManifest=read(path.join(bundle,'files.json'));
  for(const e of bundleManifest.entries.filter(e=>e.file.replace(/\\/g,'/').startsWith('data/'))){const original=path.join(representative,'model',path.basename(e.file));if(sha(original)!==e.sha256)throw Error('Model changed');fs.copyFileSync(original,path.join(data,path.basename(e.file)));}
  const dll='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll';if(sha(dll)!==bundleManifest.external_dependency.sha256)throw Error('FFTW changed');
  const cohortHash=sha(cohortPath);if(cohortHash!==provenance.cohort_sha256)throw Error('Cohort changed');
  const state=read(path.join(source,'checkpoint.json')),jobs=[],roots=new Map();
  if(state.videos.length!==cohort.cases.length||new Set(state.videos.map(v=>v.video)).size!==cohort.cases.length)throw Error('Incomplete accepted cohort');
  for(const c of cohort.cases){const v=state.videos.find(v=>v.video===c.video);if(!v)throw Error('Missing accepted video');const root=path.resolve(source,v.root);accepted(root,c,dataset);roots.set(c.video,root);const plan=read(path.join(root,'plan.json'));for(const index of plan.indices)jobs.push({video:c.video,index,frame:path.join(root,'frame'+String(index).padStart(4,'0'))});}
  total=jobs.length;
  const manifest={cohort_sha256:cohortHash,reference_runs:'preserved accepted official records; not recomputed in this backend sweep',historical_discrepancies:config.unresolved_reference_discrepancies||[],runtime:fs.readdirSync(runtime).filter(f=>f!=='model').map(file=>({file,sha256:sha(path.join(runtime,file))})),jobs};
  fs.writeFileSync(path.join(out,'plan.json'),JSON.stringify(manifest,null,2));save('evaluating_all_frames');log('START '+total+' frames, workers=2; representative frames are freshly recomputed');
  let next=0,failed=false;
  const run=args=>new Promise((resolve,reject)=>{const child=cp.spawn(process.execPath,args,{stdio:'inherit',windowsHide:true});child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error('Frame wrapper exited '+code)));});
  const worker=async()=>{try{while(!failed&&next<jobs.length){const job=jobs[next++],name=job.video+'_frame'+String(job.index).padStart(4,'0'),destination=path.join(out,name),stock=path.join(job.frame,'official/score.json');
   if(sha(cohortPath)!==cohortHash)throw Error('Cohort changed during sweep');
   log(name+' start');await run(['tests/run_live_hdr_mutual_experimental.cjs',path.join(runtime,'experimental.exe'),path.join(job.frame,'decoded/reference.png'),path.join(job.frame,'decoded/test.png'),data,dll,path.join(runtime,'projection.spv'),path.join(runtime,'mask.spv'),path.join(runtime,'mutual.spv'),stock,destination]);
   const comparison=read(path.join(destination,'comparison.json'));if(!comparison.exact||!comparison.backendVerified||comparison.validationErrors)throw Error('Frame not accepted');
   results.push({video:job.video,index:job.index,root:path.relative(out,destination),Q_hex:comparison.Q_hex,Q_JOD_hex:comparison.Q_JOD_hex});
   fs.appendFileSync(path.join(out,'accepted_frames.jsonl'),JSON.stringify(results.at(-1))+'\n');save('evaluating_all_frames');log(name+' EXACT '+results.length+'/'+total);
  }}catch(e){failed=true;throw e;}};
  const workers=await Promise.allSettled([worker(),worker()]);const errors=workers.filter(r=>r.status==='rejected');if(errors.length)throw errors[0].reason;
  if(results.length!==total)throw Error('Incomplete full-frame sweep');
  const word=Buffer.alloc(4),number=hex=>{word.writeUInt32LE(parseInt(hex,16));return word.readFloatLE();},hex=value=>{word.writeFloatLE(value);return word.readUInt32LE().toString(16).padStart(8,'0');};
  const videos=[];for(const c of cohort.cases){const rows=results.filter(r=>r.video===c.video).sort((a,b)=>a.index-b.index);let sum=0;for(const r of rows)sum=Math.fround(sum+number(r.Q_JOD_hex));const actual=Math.fround(sum/rows.length),expected=read(path.join(roots.get(c.video),'video_score.json')).official_Q_JOD;if(hex(actual)!==hex(expected))throw Error('Video mean differs');videos.push({video:c.video,frames:rows.length,Q_JOD:actual,Q_JOD_hex:hex(actual),exact:true});}
  fs.writeFileSync(path.join(out,'video_means.json'),JSON.stringify({scope:'full accepted local cohort with frozen new hybrid backend',videos},null,2));save('complete',{videos:videos.length});log('COMPLETE all frames and native Single video means exact; historical discrepancy remains open');
 }catch(e){save('failed',{error:String(e.stack)});throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
