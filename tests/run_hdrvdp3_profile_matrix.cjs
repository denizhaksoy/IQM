// Extend numerical coverage beyond the fixed 4K LIVE profile, with explicit PPD.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function main(){
 const [officialArg,bundleArg,cpuArg,mutualRuntimeArg,outArg,selectionArg,resizeMode]=process.argv.slice(2);
 const selection=selectionArg==='all'?undefined:selectionArg;
 if(resizeMode!==undefined&&resizeMode!=='resize')throw Error('Optional resize mode must be explicit: resize');
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: clean-reference bundle CPU-exe accepted-mutual-runtime fresh-out');
 const official=path.resolve(officialArg),bundle=path.resolve(bundleArg),sourceRuntime=path.resolve(mutualRuntimeArg),out=path.resolve(outArg);
 const octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',dll=path.join(path.dirname(octave),'libfftw3f-3.dll'),manifest=read(path.join(bundle,'files.json'));
 if(sha(path.join(official,'hdrvdp3.m'))!=='0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a'||sha(dll)!==manifest.external_dependency.sha256)throw Error('Unexpected official/FFTW runtime');
 fs.mkdirSync(out);const runtime=path.join(out,'runtime');fs.mkdirSync(runtime);fs.mkdirSync(path.join(runtime,'model'));
 fs.copyFileSync(cpuArg,path.join(runtime,'cpu.exe'));
 for(const f of ['experimental.exe','projection.spv','mask.spv','mutual.spv'])fs.copyFileSync(path.join(sourceRuntime,f),path.join(runtime,f));
 if(resizeMode)fs.copyFileSync(path.join(sourceRuntime,'resize.spv'),path.join(runtime,'resize.spv'));
 for(const e of manifest.entries.filter(e=>e.file.replace(/\\/g,'/').startsWith('data/'))){const file=path.join(bundle,e.file);if(sha(file)!==e.sha256)throw Error('Model changed');fs.copyFileSync(file,path.join(runtime,'model',path.basename(e.file)));}
 const referenceHash=fingerprintTree(official),results=[];
 const provenance={scope:'synthetic dimensions/PPD stress tests; not paper reproduction',selection:selection||'all',reference_tree_sha256:referenceHash,octave_sha256:sha(octave),fftw_sha256:sha(dll),runtime:fs.readdirSync(runtime).filter(f=>f!=='model').map(file=>({file,sha256:sha(path.join(runtime,file))})),scripts:['tests/generate_hdrvdp3_profile_fixtures.m','tests/prepare_hdrvdp3_quality_profile.m','tests/score_live_hdr_reference_frame.m','tests/run_hdrvdp3_profile_matrix.cjs'].map(file=>({file,sha256:sha(file)})),profile:{task:'quality',encoding:'pq-bt2020-rgb16',precision:'single',age:24,display:'oled',ambient_lux:200,reflectivity:0.005,peak:2000,fftw_threads:3,fftw_planner:'estimate',padding:'unchanged native pipeline per geometry',DC:'unchanged base-band zeroing'}};
 provenance.comparison_resize_enabled=Boolean(resizeMode);
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify(provenance,null,2));
 const run=(exe,args,file)=>{const p=cp.spawnSync(exe,args,{encoding:'utf8',windowsHide:true,maxBuffer:32*1024*1024});fs.writeFileSync(file,(p.stdout||'')+(p.stderr||''));if(p.error)throw p.error;if(p.status!==0)throw Error(exe+' exited '+p.status+'; '+file);return (p.stdout||'')+(p.stderr||'');};
 const event=s=>{console.log(s);fs.appendFileSync(path.join(out,'progress.log'),new Date().toISOString()+' '+s+'\n');};
 run(octave,['--quiet','tests/generate_hdrvdp3_profile_fixtures.m',path.join(out,'fixtures'),...(selection?[selection]:[])],path.join(out,'fixtures.log'));
 const fixtures=read(path.join(out,'fixtures/fixtures.json'));
 const save=complete=>fs.writeFileSync(path.join(out,'checkpoint.json'),JSON.stringify({time:new Date().toISOString(),complete,passed:complete&&results.every(r=>r.exact),total:fixtures.cases.length,results},null,2));save(false);
 for(const c of fixtures.cases){const job=path.join(out,c.name);fs.mkdirSync(job);event(c.name+' START '+c.width+'x'+c.height+' ppd='+c.ppd);
  try{
   if(sha(c.reference)!==c.reference_sha256||sha(c.test)!==c.test_sha256)throw Error('Fixture changed');
   run(octave,['--quiet','tests/prepare_hdrvdp3_quality_profile.m',official,c.reference,c.test,path.join(job,'preprocessing'),String(c.ppd)],path.join(job,'preprocessing.log'));
   run(octave,['--quiet','tests/score_live_hdr_reference_frame.m',official,path.join(job,'preprocessing'),path.join(job,'official')],path.join(job,'official.log'));
   const stock=read(path.join(job,'official/score.json'));
   if(stock.ppd!==c.ppd||stock.Q_class!=='single'||stock.Q_JOD_class!=='single'||stock.inputs[0].source_sha256!==c.reference_sha256||stock.inputs[1].source_sha256!==c.test_sha256)throw Error('Reference profile/input mismatch');
   const common=['--input',c.test,'--ref',c.reference,'--encoding','pq-bt2020-rgb16','--ppd',String(c.ppd),'--model-data',path.join(runtime,'model'),'--fftw',dll];
   run(path.join(runtime,'cpu.exe'),[...common,'--output',path.join(job,'cpu')],path.join(job,'cpu.log'));
   const gpuLog=run(path.join(runtime,'experimental.exe'),[...common,'--projection-shader',path.join(runtime,'projection.spv'),'--comparison-shader',path.join(runtime,'mask.spv'),'--mutual-shader',path.join(runtime,'mutual.spv'),...(resizeMode?['--comparison-resize-shader',path.join(runtime,'resize.spv')]:[]),'--output',path.join(job,'gpu')],path.join(job,'gpu.log'));
   const cpu=read(path.join(job,'cpu/score.json')),gpu=read(path.join(job,'gpu/score.json')),stages=read(path.join(job,'gpu/gpu_stages.json'));
   let levels=0;for(let a=c.width,b=c.height;a>=13&&b>=13;a=Math.floor(a/2),b=Math.floor(b/2))levels++;
   const bands=Math.min(Math.ceil(Math.log2(c.ppd))-2,levels)+2;
   const exact=cpu.Q_hex===stock.Q_hex&&cpu.Q_JOD_hex===stock.Q_JOD_hex&&gpu.Q_hex===stock.Q_hex&&gpu.Q_JOD_hex===stock.Q_JOD_hex;
   const backendVerified=gpu.backend===(resizeMode?'experimental-vulkan-matrices-comparison-mutual-resize-cpu-fftw':'experimental-vulkan-matrices-comparison-mutual-cpu-fftw')&&stages.matrix_dispatches===4&&stages.comparison_dispatches===1&&stages.mutual_dispatches===bands&&stages.full_gpu===false&&(!resizeMode||(stages.comparison_resize_calls===bands&&stages.comparison_resize_dispatches===2*(bands-2)));
   const validationErrors=/\[ERROR\]|Validation Error|VUID-/i.test(gpuLog);
   const result={...c,exact:exact&&backendVerified&&!validationErrors,cpu_Q_hex:cpu.Q_hex,cpu_Q_JOD_hex:cpu.Q_JOD_hex,gpu_Q_hex:gpu.Q_hex,gpu_Q_JOD_hex:gpu.Q_JOD_hex,reference_Q_hex:stock.Q_hex,reference_Q_JOD_hex:stock.Q_JOD_hex,backendVerified,mutual_dispatches:stages.mutual_dispatches,validationErrors};
   results.push(result);fs.writeFileSync(path.join(job,'comparison.json'),JSON.stringify(result,null,2));event(c.name+(result.exact?' EXACT':' MISMATCH'));
  }catch(e){results.push({...c,exact:false,error:String(e.stack)});event(c.name+' FAILED '+e.message);}
  save(false);
 }
 if(fingerprintTree(official)!==referenceHash)throw Error('Official reference changed during run');
 for(const e of provenance.runtime)if(sha(path.join(runtime,e.file))!==e.sha256)throw Error('Frozen runtime changed');
 save(true);if(results.some(r=>!r.exact))throw Error('Profile matrix has failures; original evidence preserved');
 event('COMPLETE all '+results.length+' profile cases exact');
}
try{main();}catch(e){console.error(e.stack);process.exitCode=1;}
