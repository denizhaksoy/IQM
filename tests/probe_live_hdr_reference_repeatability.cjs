// Preserve every clean-reference result; never promote a majority vote to parity.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto'),os=require('node:os');
const {fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [frameArg,officialArg,outArg,countArg='8']=process.argv.slice(2),count=Number(countArg);
 if(!outArg||fs.existsSync(outArg)||!Number.isSafeInteger(count)||count<2||count>64)throw Error('Usage: original-frame-root clean-official-root fresh-root count[2..64]');
 const frame=path.resolve(frameArg),official=path.resolve(officialArg),out=path.resolve(outArg),base=path.join(frame,'preprocessing'),original=read(path.join(frame,'official/score.json')),meta=read(path.join(base,'preprocessing.json'));
 if(sha(path.join(official,'hdrvdp3.m'))!=='0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a')throw Error('Not clean v3.0.7');
 for(const e of meta.inputs)if(sha(path.join(base,e.name+'.f32'))!==e.linear_sha256||original.inputs.find(o=>o.name===e.name)?.linear_sha256!==e.linear_sha256)throw Error('Original inputs changed');
 const octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',dll=path.join(path.dirname(octave),'libfftw3f-3.dll'),scorer='tests/score_live_hdr_reference_frame.m';
 fs.mkdirSync(out,{recursive:true});
 const provenance={official_tree_sha256:fingerprintTree(official),scorer_sha256:sha(scorer),octave_sha256:sha(octave),fftw_sha256:sha(dll),preprocessing_sha256:sha(path.join(base,'preprocessing.json')),platform:process.platform,os_release:os.release(),cpu:os.cpus()[0].model,logical_cpus:os.cpus().length,free_memory:os.freemem(),environment:Object.fromEntries(['OPENBLAS_NUM_THREADS','OMP_NUM_THREADS','MKL_NUM_THREADS','OMP_DYNAMIC','OMP_PROC_BIND'].map(k=>[k,process.env[k]??null])),original_score_sha256:sha(path.join(frame,'official/score.json')),original:{Q_hex:original.Q_hex,Q_JOD_hex:original.Q_JOD_hex}};
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify(provenance,null,2));
 const results=[];const save=()=>{const groups={};for(const r of results){const key=r.Q_hex+'/'+r.Q_JOD_hex;groups[key]=(groups[key]??0)+1;}fs.writeFileSync(path.join(out,'checkpoint.json'),JSON.stringify({scope:'clean reference repeatability investigation only; no parity acceptance or original result replacement',complete:results.length===count,requested:count,groups,results},null,2));};save();
 let cursor=0;async function worker(){while(cursor<count){const i=cursor++,name='repeat'+String(i+1).padStart(2,'0');
  // Match the original output argument length to test allocation/path sensitivity.
  const desired=path.join(frame,'official').length,minimal=path.join(out,name).length;
  const padding=Math.max(0,desired-minimal-1),target=padding?path.join(out,'p'.repeat(padding),name):path.join(out,name);
  const log=fs.createWriteStream(path.join(out,name+'.log'));
  await new Promise((resolve,reject)=>{const c=cp.spawn(octave,['--quiet',scorer,official,base,target],{windowsHide:true});c.stdout.pipe(log,{end:false});c.stderr.pipe(log,{end:false});c.on('error',reject);c.on('close',code=>{log.end();code===0?resolve():reject(Error(name+' exited '+code));});});
  const s=read(path.join(target,'score.json'));if(s.ppd!==original.ppd||s.input_precision!=='single'||s.fftw_threads!==3||s.fftw_planner!=='estimate'||s.backend!=='CPU'||s.inputs.some(e=>original.inputs.find(o=>o.name===e.name)?.linear_sha256!==e.linear_sha256))throw Error('Repeat protocol changed');
  results.push({run:i+1,root:path.relative(out,target),output_argument_length:target.length,original_output_argument_length:desired,Q_hex:s.Q_hex,Q_JOD_hex:s.Q_JOD_hex,matches_original:s.Q_hex===original.Q_hex&&s.Q_JOD_hex===original.Q_JOD_hex});save();console.log(name+' '+s.Q_hex+'/'+s.Q_JOD_hex+' original='+results.at(-1).matches_original);
 }}
 await Promise.all(Array.from({length:2},worker));
 if(fingerprintTree(official)!==provenance.official_tree_sha256||sha(scorer)!==provenance.scorer_sha256||sha(octave)!==provenance.octave_sha256||sha(dll)!==provenance.fftw_sha256)throw Error('Reference runtime changed during probe');
 fs.writeFileSync(path.join(out,'runtime_unchanged.json'),JSON.stringify({verified:true}));
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
