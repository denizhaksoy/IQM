// Representative frame0 from every accepted cohort video through both public CLIs.
// This does not replace full-video reference validation or establish full GPU parity.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [configFile,cpu,hybrid,shader,out]=process.argv.slice(2);
 if(!out||fs.existsSync(out))throw Error('Usage: scheduler-config CPU-exe hybrid-exe projection-shader fresh-root');
 const config=read(configFile),base=path.dirname(path.resolve(configFile)),resolve=p=>path.resolve(base,p),cohort=read(resolve(config.cohort)),dataset=resolve(config.dataset),source=resolve(config.output);
 const dll='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll',bundle=resolve(config.bundle),manifest=read(path.join(bundle,'files.json'));
 if(sha(dll)!==manifest.external_dependency.sha256)throw Error('Unexpected FFTW runtime');
 fs.mkdirSync(out,{recursive:true});fs.mkdirSync(path.join(out,'model'));
 const frozenCpu=path.resolve(out,'cpu.exe'),frozenHybrid=path.resolve(out,'hybrid.exe'),frozenShader=path.resolve(out,'projection.spv');
 fs.copyFileSync(cpu,frozenCpu);fs.copyFileSync(hybrid,frozenHybrid);fs.copyFileSync(shader,frozenShader);
 for(const e of manifest.entries.filter(e=>e.file.replace(/\\/g,'/').startsWith('data/'))){const file=path.join(bundle,e.file);if(sha(file)!==e.sha256)throw Error('Model data changed');fs.copyFileSync(file,path.join(out,'model',path.basename(e.file)));}
 const provenance={cpu_sha256:sha(frozenCpu),hybrid_sha256:sha(frozenHybrid),shader_sha256:sha(frozenShader),fftw_sha256:sha(dll),cohort_sha256:sha(resolve(config.cohort)),selection:'frame0 of each accepted video; does not add unique cases'};
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify(provenance,null,2));
 const results=[],save=()=>{const temporary=path.join(out,'checkpoint.tmp');fs.writeFileSync(temporary,JSON.stringify({scope:'representative public CPU/hybrid backend checks, not full-video GPU or original-paper reproduction',complete:results.length===cohort.cases.length,total:cohort.cases.length,results},null,2));fs.renameSync(temporary,path.join(out,'checkpoint.json'));};
 const run=args=>new Promise((resolve,reject)=>{const c=cp.spawn(process.execPath,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error('Public backend audit exited '+code)));});
 save();let idle=0;
 while(results.length<cohort.cases.length){
  const state=read(path.join(source,'checkpoint.json')),pending=state.videos.filter(v=>!results.some(r=>r.video===v.video));
  for(const v of pending){
   const c=cohort.cases.find(c=>c.video===v.video);if(!c)throw Error('Video outside declared cohort');
   const root=path.resolve(source,v.root);accepted(root,c,dataset);
   if(sha(dll)!==provenance.fftw_sha256||sha(resolve(config.cohort))!==provenance.cohort_sha256)throw Error('Runtime/cohort changed');
   const frame=path.join(root,'frame0000'),r=read(path.join(frame,'comparison.json')),job=path.join(out,c.video);fs.mkdirSync(job);
   const stock=path.resolve(job,'official_score.json');fs.writeFileSync(stock,JSON.stringify(r.official));
   const ref=path.resolve(frame,'decoded/reference.png'),test=path.resolve(frame,'decoded/test.png'),data=path.resolve(out,'model');
   await run(['tests/run_live_hdr_cpu.cjs',frozenCpu,ref,test,data,dll,stock,path.join(job,'cpu')]);
   await run(['tests/run_live_hdr_hybrid.cjs',frozenHybrid,ref,test,data,dll,frozenShader,stock,path.join(job,'hybrid')]);
   const cpuResult=read(path.join(job,'cpu/comparison.json')),gpuResult=read(path.join(job,'hybrid/comparison.json'));
   if(!cpuResult.exact||!cpuResult.settings_verified||!gpuResult.exact||!gpuResult.backendVerified||gpuResult.validationErrors)throw Error('Public backend acceptance failed');
   results.push({video:c.video,frame:0,cpu_exact:true,hybrid_exact:true,gpu_matrix_dispatches:4,Q_hex:r.official.Q_hex,Q_JOD_hex:r.official.Q_JOD_hex});save();console.log('PUBLIC BACKENDS '+results.length+'/'+cohort.cases.length+' exact');
  }
  if(results.length===cohort.cases.length)break;
  const lock=path.join(source,'scheduler.lock');if(fs.existsSync(lock)){process.kill(read(lock).pid,0);idle=0;}else if(++idle>6)throw Error('Incomplete cohort and scheduler unavailable');
  await new Promise(resolve=>setTimeout(resolve,5000));
 }
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
