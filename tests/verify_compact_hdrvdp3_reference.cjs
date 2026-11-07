const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const {fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [officialArg,matrix1Arg,matrix2Arg,outArg]=process.argv.slice(2);
 if(!outArg||fs.existsSync(outArg))throw Error('Usage: clean-official twelve-case-root two-case-root fresh-out');
 const official=path.resolve(officialArg),out=path.resolve(outArg),cases=[];
 for(const arg of [matrix1Arg,matrix2Arg]){const root=path.resolve(arg),p=read(path.join(root,'checkpoint.json'));if(!p.complete||!p.passed)throw Error('Original case matrix not accepted');for(const c of p.results)cases.push({name:c.name,reference:c.reference,test:c.test,stock:path.join(root,c.name,'official/score.json'),ppd:c.ppd});}
 const football=path.resolve('validation/live_hdr_cohort_scores_20261102_run01/results/540p_2.2M_football1_1793653626058/frame0150');
 cases.push({name:'football150_later_stock',reference:path.join(football,'decoded/reference.png'),test:path.join(football,'decoded/test.png'),stock:path.resolve('validation/live_hdr_failure_stock_repeat_20261102_run01/score.json'),ppd:62.729130063099447});
 const daylight=path.resolve('validation/live_hdr_video_20261101_run01/frame0000'),daylightScore=read(path.join(daylight,'comparison.json')).official;
 fs.mkdirSync(out);const daylightStock=path.join(out,'preserved_daylight_stock.json');fs.writeFileSync(daylightStock,JSON.stringify(daylightScore));
 cases.push({name:'daylight0',reference:path.join(daylight,'decoded/reference.png'),test:path.join(daylight,'decoded/test.png'),stock:daylightStock,ppd:62.729130063099447});
 const octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',tree=fingerprintTree(official),script='tests/score_hdrvdp3_compact_reference.m';
 fs.copyFileSync(script,path.join(out,'score_hdrvdp3_compact_reference.m'));fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify({scope:'compact-vs-original official workflow equivalence; old historical outlier remains open',reference_tree_sha256:tree,octave_sha256:sha(octave),script_sha256:sha(script),cases:cases.map(c=>({...c,stock_sha256:sha(c.stock),reference_sha256:sha(c.reference),test_sha256:sha(c.test)}))},null,2));
 const results=[],save=complete=>{const temp=path.join(out,'checkpoint.tmp');fs.writeFileSync(temp,JSON.stringify({time:new Date().toISOString(),complete,passed:complete&&results.every(r=>r.exact),total:cases.length,completed:results.length,results,historical_reference_discrepancy_resolved:false},null,2));fs.renameSync(temp,path.join(out,'checkpoint.json'));};save(false);
 const run=(c,destination,log)=>new Promise((resolve,reject)=>{const child=cp.spawn(octave,['--quiet',script,official,c.reference,c.test,destination,String(c.ppd)],{windowsHide:true});const write=b=>fs.appendFileSync(log,b);child.stdout.on('data',write);child.stderr.on('data',write);child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error('Reference exited '+code+'; '+log)));});
 let next=0;const worker=async()=>{while(next<cases.length){const c=cases[next++],destination=path.join(out,c.name);try{
  await run(c,destination,path.join(out,c.name+'.log'));const expected=read(c.stock),actual=read(path.join(destination,'score.json'));
  // Old JSON serialization changed decimal spellings of Single extrema. Require
  // identical Float32 bits (not a tolerance), plus identical full-image hashes.
  const f32hex=value=>{const b=Buffer.alloc(4);b.writeFloatLE(value);return b.toString('hex');};
  const inputsExact=actual.inputs.length===expected.inputs.length&&actual.inputs.every((a,i)=>['name','source_sha256','linear_sha256','unit_class','linear_class','values'].every(k=>a[k]===expected.inputs[i][k])&&['minimum','maximum'].every(k=>Number.isFinite(a[k])&&Number.isFinite(expected.inputs[i][k])&&f32hex(a[k])===f32hex(expected.inputs[i][k])));
  const scoresExact=actual.Q_hex===expected.Q_hex&&actual.Q_JOD_hex===expected.Q_JOD_hex;
  const profileExact=['reference_version','backend','fftw_threads','fftw_planner','input_precision','ppd','Q_class','Q_JOD_class'].every(k=>actual[k]===expected[k]);
  const noLinearExports=!fs.readdirSync(destination).some(f=>f.endsWith('.f32'))&&actual.persistent_linear_exports===false;
  const exact=inputsExact&&scoresExact&&profileExact&&noLinearExports;results.push({name:c.name,exact,inputsExact,scoresExact,profileExact,noLinearExports,Q_hex:actual.Q_hex,Q_JOD_hex:actual.Q_JOD_hex,expected_Q_hex:expected.Q_hex,expected_Q_JOD_hex:expected.Q_JOD_hex});console.log(c.name+(exact?' EXACT':' MISMATCH'));
 }catch(e){results.push({name:c.name,exact:false,error:String(e.stack)});console.log(c.name+' FAILED '+e.message);}save(false);}};
 await Promise.all([worker(),worker()]);if(fingerprintTree(official)!==tree)throw Error('Clean official source changed');save(true);if(results.some(r=>!r.exact))throw Error('Compact workflow not accepted; all evidence retained');console.log('COMPLETE '+results.length+' compact reference cases exact, no Float32 export files');
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
