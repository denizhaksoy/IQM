// Representative end-to-end regression for the explicit experimental comparison+mutual profile.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [configFile,exe,projection,mask,mutual,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: completed-cohort-config experimental-exe projection.spv mask.spv mutual.spv fresh-out');
 const out=path.resolve(outArg),base=path.dirname(path.resolve(configFile)),config=read(configFile),resolve=p=>path.resolve(base,p),cohortPath=resolve(config.cohort),cohort=read(cohortPath),source=resolve(config.output),dataset=resolve(config.dataset),state=read(path.join(source,'checkpoint.json'));
 if(state.videos.length!==cohort.cases.length||new Set(state.videos.map(v=>v.video)).size!==cohort.cases.length)throw Error('Requires complete unique local cohort');
 const bundle=resolve(config.bundle),manifest=read(path.join(bundle,'files.json')),dll='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll';
 if(sha(dll)!==manifest.external_dependency.sha256)throw Error('FFTW changed');
 fs.mkdirSync(out);fs.mkdirSync(path.join(out,'model'));
 const frozenExe=path.join(out,'experimental.exe'),frozenProjection=path.join(out,'projection.spv'),frozenMask=path.join(out,'mask.spv'),frozenMutual=path.join(out,'mutual.spv');
 for(const [a,b] of [[exe,frozenExe],[projection,frozenProjection],[mask,frozenMask],[mutual,frozenMutual]])fs.copyFileSync(a,b);
 for(const e of manifest.entries.filter(e=>e.file.replace(/\\/g,'/').startsWith('data/'))){const file=path.join(bundle,e.file);if(sha(file)!==e.sha256)throw Error('Model changed');fs.copyFileSync(file,path.join(out,'model',path.basename(e.file)));}
 const fingerprint=sha(cohortPath),results=[];
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify({executable_sha256:sha(frozenExe),projection_sha256:sha(frozenProjection),mask_sha256:sha(frozenMask),mutual_sha256:sha(frozenMutual),cohort_sha256:fingerprint,selection:'frame0 of each video; repeats existing cases, not additional unique coverage',historical_discrepancies:config.unresolved_reference_discrepancies||[]},null,2));
 const save=(stage,error)=>{const temp=path.join(out,'checkpoint.tmp');fs.writeFileSync(temp,JSON.stringify({time:new Date().toISOString(),scope:'experimental matrices+comparison+mutual backend representative frames only; remaining operations CPU',stage,complete:stage==='complete',total:cohort.cases.length,results,error},null,2));fs.renameSync(temp,path.join(out,'checkpoint.json'));};
 const run=args=>new Promise((resolve,reject)=>{const c=cp.spawn(process.execPath,args,{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error('Mask backend exited '+code)));});
 save('running');
 try{for(const c of cohort.cases){
  const v=state.videos.find(v=>v.video===c.video);if(!v)throw Error('Missing video');
  const root=path.resolve(source,v.root);accepted(root,c,dataset);if(sha(cohortPath)!==fingerprint)throw Error('Cohort changed');
  const frame=path.join(root,'frame0000'),r=read(path.join(frame,'comparison.json')),job=path.join(out,c.video);fs.mkdirSync(job);
  const stock=path.join(job,'official_score.json');fs.writeFileSync(stock,JSON.stringify(r.official));
  await run(['tests/run_live_hdr_mutual_experimental.cjs',frozenExe,path.join(frame,'decoded/reference.png'),path.join(frame,'decoded/test.png'),path.join(out,'model'),dll,frozenProjection,frozenMask,frozenMutual,stock,path.join(job,'evaluation')]);
  const result=read(path.join(job,'evaluation/comparison.json'));if(!result.exact||!result.backendVerified||result.validationErrors)throw Error('Mask backend failed');
  results.push({video:c.video,frame:0,exact:true,Q_hex:result.Q_hex,Q_JOD_hex:result.Q_JOD_hex,gpu_matrix_dispatches:4,gpu_comparison_dispatches:1,gpu_mutual_dispatches:6});save('running');
 }save('complete');}catch(e){save('failed',String(e.stack));throw e;}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
