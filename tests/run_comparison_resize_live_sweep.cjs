// Validate only the changed Double resize stage on masks from accepted real
// inputs; do not rerun unrelated FFT/JND/quality calculations or claim new scores.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [acceptedArg,extraArg,runtimeArg,exeArg,matrixArg,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: completed-513-root completed-extra-video-root resize-runtime probe-exe passed-resize-matrix fresh-out');
 const accepted=path.resolve(acceptedArg),extra=path.resolve(extraArg),sourceRuntime=path.resolve(runtimeArg),matrix=path.resolve(matrixArg),out=path.resolve(outArg);
 fs.mkdirSync(out);const status=path.join(out,'status.json'),save=(stage,fields={})=>{const temp=status+'.tmp';fs.writeFileSync(temp,JSON.stringify({time:new Date().toISOString(),stage,scope:'Double comparison resize stage only; not a full-score rerun or original paper reproduction',...fields},null,2));fs.renameSync(temp,status);};
 const run=(exe,args,log)=>new Promise((resolve,reject)=>{const child=cp.spawn(exe,args,{windowsHide:true});const append=b=>fs.appendFileSync(log,b);child.stdout.on('data',append);child.stderr.on('data',append);child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error('Probe exited '+code+'; '+log)));});
 try{
  const a=read(path.join(accepted,'checkpoint.json')),b=read(path.join(extra,'checkpoint.json')),m=read(path.join(matrix,'checkpoint.json'));
  if(!a.complete||a.stage!=='complete'||!b.complete||b.stage!=='complete'||!m.complete||!m.passed||m.results.some(r=>!r.exact))throw Error('Required preceding validation not complete');
  const runtime=path.join(out,'runtime');fs.mkdirSync(runtime);fs.mkdirSync(path.join(runtime,'model'));fs.copyFileSync(exeArg,path.join(runtime,'probe.exe'));fs.copyFileSync(path.join(sourceRuntime,'resize.spv'),path.join(runtime,'resize.spv'));
  const mp=read(path.join(matrix,'provenance.json')),shader=mp.runtime.find(r=>r.file==='resize.spv');if(!shader||sha(path.join(runtime,'resize.spv'))!==shader.sha256)throw Error('Unvalidated resize shader');
  const modelEntries=fs.readdirSync(path.join(matrix,'runtime/model')).map(file=>({file,sha256:sha(path.join(matrix,'runtime/model',file))}));
  for(const e of modelEntries){const file=path.join(sourceRuntime,'model',e.file);if(sha(file)!==e.sha256)throw Error('Model differs');fs.copyFileSync(file,path.join(runtime,'model',e.file));}
  const extraSource=read(path.join(extra,'provenance.json')).source,requests=[];
  for(const j of read(path.join(accepted,'plan.json')).jobs)requests.push({id:j.video+'_frame'+String(j.index).padStart(4,'0'),frame:j.frame});
  for(const r of b.results)requests.push({id:b.video+'_frame'+String(r.index).padStart(4,'0'),frame:path.join(extraSource,'frame'+String(r.index).padStart(4,'0'))});
  if(requests.length!==a.total_frames+b.total)throw Error('Frame count differs');
  const sources=[],pairs=new Set();save('verifying_inputs',{total:requests.length,verified:0});
  for(const request of requests){const record=read(path.join(request.frame,'comparison.json'));if(!record.exact||record.official.ppd!==62.729130063099447||record.official.Q_hex!==record.cpp.Q_hex||record.official.Q_JOD_hex!==record.cpp.Q_JOD_hex||!/^[A-Za-z0-9_.-]+$/.test(request.id))throw Error('Source acceptance differs');
   const reference=path.resolve(request.frame,'decoded/reference.png'),test=path.resolve(request.frame,'decoded/test.png'),refHash=sha(reference),testHash=sha(test);
   if(refHash!==record.official.inputs[0].source_sha256||testHash!==record.official.inputs[1].source_sha256||[reference,test].some(p=>/[\t\r\n]/.test(p)))throw Error('PNG source differs');
   const key=refHash+':'+testHash;if(pairs.has(key))throw Error('Duplicate input pair');pairs.add(key);sources.push({...request,reference,test,reference_sha256:refHash,test_sha256:testHash});
   if(sources.length%32===0)save('verifying_inputs',{total:requests.length,verified:sources.length});
  }
  const list=path.join(out,'cases.tsv');fs.writeFileSync(list,sources.map(s=>[s.id,s.reference,s.test].join('\t')).join('\n')+'\n');
  const scripts=['tests/verify_live_comparison_resize.cpp','tests/run_comparison_resize_live_sweep.cjs','src/hdrvdp3/comparison_resize_gpu.cpp','src/hdrvdp3/resize.cpp','src/hdrvdp3/pq_single.cpp','src/hdrvdp3/linear_color_single.cpp','src/hdrvdp3/spectral.cpp','src/hdrvdp3/spectral_loader.cpp','shaders/hdrvdp3/hdrvdp3_comparison_resize.comp'];
  fs.mkdirSync(path.join(out,'sources'));for(const file of scripts)fs.copyFileSync(file,path.join(out,'sources',path.basename(file)));
  const provenance={scope:'CPU/GPU Double mask-resize values from independently accepted real PNG pairs; no FFT/DC stage and no new quality predictions',inputs:sources,model:modelEntries,exe_sha256:sha(path.join(runtime,'probe.exe')),shader_sha256:sha(path.join(runtime,'resize.spv')),cases_sha256:sha(list),source_snapshots:scripts.map(file=>({file,sha256:sha(file)}))};fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify(provenance,null,2));
  save('evaluating_resize_masks',{total:sources.length});console.log('START '+sources.length+' unique real masks, changed stage only');
  const resultRoot=path.join(out,'results');await run(path.join(runtime,'probe.exe'),[list,path.join(runtime,'model'),path.join(runtime,'resize.spv'),resultRoot],path.join(out,'probe.log'));
  const result=read(path.join(resultRoot,'checkpoint.json')),rows=fs.readFileSync(path.join(resultRoot,'results.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse);
  if(!result.complete||!result.passed||result.completed_frames!==sources.length||rows.length!==sources.length||new Set(rows.map(r=>r.case)).size!==sources.length||rows.some(r=>!r.exact||r.dispatches!==8))throw Error('Incomplete resize-stage results');
  const log=fs.readFileSync(path.join(out,'probe.log'),'utf8');if(/\[ERROR\]|Validation Error|VUID-/i.test(log))throw Error('Vulkan validation error');
  save('verifying_input_preservation',{total:sources.length});
  for(const s of sources)if(sha(s.reference)!==s.reference_sha256||sha(s.test)!==s.test_sha256)throw Error('PNG changed during sweep');
  for(const e of modelEntries)if(sha(path.join(runtime,'model',e.file))!==e.sha256)throw Error('Model changed');
  if(sha(path.join(runtime,'probe.exe'))!==provenance.exe_sha256||sha(path.join(runtime,'resize.spv'))!==provenance.shader_sha256||sha(list)!==provenance.cases_sha256)throw Error('Runtime changed');
  save('complete',{complete:true,passed:true,unique_frames:sources.length,double_values:result.double_values,full_gpu_verified:false,original_paper_reproduction_verified:false});console.log('COMPLETE changed resize stage exact on '+sources.length+' real masks');
 }catch(e){save('failed',{error:String(e.stack)});throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
