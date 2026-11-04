// Local Section-6 reconstruction on one pair. Explicit phase=0 and RGB48.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto'),os=require('node:os');
const {reuseFrame,fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const {referenceConsistency}=require('./live_hdr_reference_consistency.cjs');
const [dataset,official,bundle,out,testId='4k_3M_Daylight1',referenceId='4k_ref_Daylight1',seedArg,referenceRunsArg='1',priorityArg]=process.argv.slice(2);
const seedRoot=seedArg==='-'?undefined:seedArg,referenceRuns=Number(referenceRunsArg);
if(!Number.isSafeInteger(referenceRuns)||referenceRuns<1||referenceRuns>5)throw Error('Reference runs must be1..5');
if(!out||fs.existsSync(out))throw Error('Usage: staged-dataset clean-reference accepted-bundle fresh-root');
fs.mkdirSync(out,{recursive:true});
const octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe';
const dll='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll';
const exe=path.resolve(bundle,'live-frame-single.exe'),data=path.resolve(bundle,'data');
if(![testId,referenceId].every(s=>/^[A-Za-z0-9_.-]+$/.test(s)))throw Error('Video identifiers must be basenames without extension');
const reference=path.resolve(dataset,'sample_pair',referenceId+'.mp4'),test=path.resolve(dataset,'sample_pair',testId+'.mp4');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const manifest=JSON.parse(fs.readFileSync(path.join(bundle,'files.json'),'utf8'));
for(const e of manifest.entries)if(sha(path.join(bundle,e.file))!==e.sha256)throw Error('Bundle hash mismatch');
if(sha(dll)!==manifest.external_dependency.sha256)throw Error('Float FFTW differs');
const ppd=62.729130063099447;
const event=message=>{console.log(message);fs.appendFileSync(path.join(out,'progress.log'),new Date().toISOString()+' '+message+'\n');};
function run(command,args,log){return new Promise((resolve,reject)=>{
 const output=fs.createWriteStream(log);const child=cp.spawn(command,args,{windowsHide:true});
 child.stdout.pipe(output,{end:false});child.stderr.pipe(output,{end:false});
 child.on('error',reject);child.on('close',code=>{output.end();code===0?resolve():reject(Error(command+' exited '+code+'; '+log));});
});}
function timing(file){const args=['-v','error','-select_streams','v:0','-show_entries','frame=best_effort_timestamp_time','-of','json',file];
 const r=cp.spawnSync('ffprobe',args,{encoding:'utf8',windowsHide:true,maxBuffer:8*1024*1024});if(r.status!==0)throw Error(r.stderr);
 const frames=JSON.parse(r.stdout).frames;const times=frames.map(f=>f.best_effort_timestamp_time);if(times.some(t=>typeof t!=='string'||!Number.isFinite(Number(t))))throw Error('Missing frame timing');return times;}
async function main(){
 const csvPath=path.join(dataset,'metadata/video_info_with_bitrate.csv'),csv=fs.readFileSync(csvPath,'utf8').replace(/^\uFEFF/,'').replace(/\r/g,'');if(csv.includes('"'))throw Error('Quoted metadata requires full CSV parser');const lines=csv.trim().split('\n').map(s=>s.split(',')),header=lines.shift();const matches=lines.map(row=>Object.fromEntries(header.map((h,i)=>[h,row[i]]))).filter(r=>r.video===testId);
 if(matches.length!==1||matches[0].ref!==referenceId||matches[0].frm_cnt!==matches[0].ref_frm_cnt)throw Error('Unverified metadata pairing');const expectedFrames=Number(matches[0].frm_cnt);if(!Number.isSafeInteger(expectedFrames)||expectedFrames<=0)throw Error('Invalid metadata frame count');
 const rt=timing(reference),tt=timing(test);if(rt.length!==expectedFrames||tt.length!==rt.length||rt.some((t,i)=>t!==tt[i]))throw Error('Decoded timing/count mismatch');
 const indices=Array.from({length:Math.ceil(rt.length/30)},(_,i)=>i*30);
 const priority=priorityArg===undefined?null:Number(priorityArg);
 if(priority!==null&&!indices.includes(priority))throw Error('Priority frame must be one of the declared samples');
 const workIndices=priority===null?indices:[priority,...indices.filter(i=>i!==priority)];
 const calculation={reference_runs:referenceRuns,official_tree_sha256:fingerprintTree(official),bundle_manifest_sha256:sha(path.join(bundle,'files.json')),scripts:Object.fromEntries(['tests/prepare_live_hdr_reference.m','tests/score_live_hdr_reference_frame.m','tests/decode_live_hdr_frame.cjs','tests/run_live_hdr_video_pair.cjs','tests/live_hdr_frame_reuse.cjs','tests/live_hdr_reference_consistency.cjs'].map(p=>[p,sha(p)]))};
 const calculationHash=crypto.createHash('sha256').update(JSON.stringify(calculation)).digest('hex');
 const plan={scope:'one video pair local reconstruction; author phase/PNG precision unresolved',reference_id:referenceId,test_id:testId,metadata_sha256:sha(csvPath),reference,test,reference_sha256:sha(reference),test_sha256:sha(test),decoded_frames:rt.length,reference_timestamps:rt,test_timestamps:tt,zero_based_phase:0,stride:30,indices,png_pixel_format:'rgb48be',ppd,executable_sha256:sha(exe),fftw_sha256:sha(dll),calculation_sha256:calculationHash,calculation,seed_root:seedRoot?path.resolve(seedRoot):null};
 plan.reference_runs=referenceRuns;plan.processing_order=workIndices;
 fs.writeFileSync(path.join(out,'plan.json'),JSON.stringify(plan,null,2));
 const results=new Map();let cursor=0,failed=false;const workers=os.freemem()>14*1073741824?2:1;event('aligned '+rt.length+' frames; workers='+workers+'; selected='+indices.length);
 async function worker(){while(!failed&&cursor<workIndices.length){const index=workIndices[cursor++],name='frame'+String(index).padStart(4,'0'),root=path.join(out,name);fs.mkdirSync(root);event(name+' decode');
  await run(process.execPath,['tests/decode_live_hdr_frame.cjs',reference,test,String(index),path.join(root,'decoded')],path.join(root,'decode_runner.log'));
  const ref=path.join(root,'decoded/reference.png'),dis=path.join(root,'decoded/test.png');
  const known=path.join(bundle,name),resumed=referenceRuns===1?reuseFrame(seedRoot,plan,index,ref,dis):null;let stock,actual,reused=false;
  if(resumed){stock=resumed.official;actual=resumed.cpp;reused=true;event(name+' reused verified prior frame');}
  else if(referenceRuns===1&&fs.existsSync(known)&&sha(ref)===sha(path.join(known,'reference.png'))&&sha(dis)===sha(path.join(known,'test.png'))){
   const knownRuns=fs.readdirSync(bundle).filter(n=>n.startsWith('results_'+name+'_'));if(knownRuns.length!==1)throw Error('Ambiguous accepted results');
   const previous=path.join(bundle,knownRuns[0]);if(!JSON.parse(fs.readFileSync(path.join(previous,'comparison.json'),'utf8')).exact)throw Error('Reuse not exact');
   stock=JSON.parse(fs.readFileSync(path.join(known,'stock_score.json'),'utf8'));actual=JSON.parse(fs.readFileSync(path.join(previous,'score.json'),'utf8'));reused=true;
  }else{
   event(name+' prepare');await run(octave,['--quiet','tests/prepare_live_hdr_reference.m',official,ref,dis,path.join(root,'preprocessing')],path.join(root,'prepare.log'));
   event(name+' reference+C++');
   await Promise.all([
    run(octave,['--quiet','tests/score_live_hdr_reference_frame.m',official,path.join(root,'preprocessing'),path.join(root,'official')],path.join(root,'official_runner.log')),
    run(exe,[ref,dis,data,dll,String(ppd),path.join(root,'cpp')],path.join(root,'cpp_runner.log'))
   ]);
   stock=JSON.parse(fs.readFileSync(path.join(root,'official/score.json'),'utf8'));actual=JSON.parse(fs.readFileSync(path.join(root,'cpp/score.json'),'utf8'));
  }
  const observations=[stock],observationFiles=[];
  if(referenceRuns>1){
   observationFiles.push({file:'official/score.json',sha256:sha(path.join(root,'official/score.json'))});
   for(let repeat=2;repeat<=referenceRuns;repeat++){
    event(name+' reference repeat '+repeat+'/'+referenceRuns);
    const directory='official_repeat_'+repeat;
    await run(octave,['--quiet','tests/score_live_hdr_reference_frame.m',official,path.join(root,'preprocessing'),path.join(root,directory)],path.join(root,directory+'.log'));
    const file=path.join(root,directory,'score.json');observations.push(JSON.parse(fs.readFileSync(file,'utf8')));observationFiles.push({file:directory+'/score.json',sha256:sha(file)});
   }
  }
  const consistency=referenceConsistency(observations);
  if(referenceRuns>1)fs.writeFileSync(path.join(root,'reference_consistency.json'),JSON.stringify({...consistency,files:observationFiles},null,2));
  const exact=consistency.all_identical&&stock.Q_hex===actual.Q_hex&&stock.Q_JOD_hex===actual.Q_JOD_hex;
  const result={zero_based_frame:index,timestamp:rt[index],reused,exact,reference_consistency:consistency,reference_png_sha256:sha(ref),test_png_sha256:sha(dis),official:stock,cpp:actual,reuse_source:resumed?{file:resumed.source_comparison,sha256:resumed.source_comparison_sha256}:null};
  fs.writeFileSync(path.join(root,'comparison.json'),JSON.stringify(result,null,2));results.set(index,result);event(name+(exact?' EXACT':' BIT MISMATCH'));
  if(!exact)throw Error(name+(consistency.all_identical?' numerical mismatch':' inconsistent clean reference observations'));
 }}
 // Let already-running frames finish and retain their exports, but never
 // schedule more work after a failed frame. Propagate failure after draining.
 const settled=await Promise.allSettled(Array.from({length:workers},async()=>{try{await worker();}catch(error){failed=true;throw error;}}));
 const rejection=settled.find(r=>r.status==='rejected');if(rejection)throw rejection.reason;
 const ordered=indices.map(i=>results.get(i));if(ordered.length!==indices.length||ordered.some(r=>!r?.exact))throw Error('Incomplete video');
 const value=hex=>{if(!/^[0-9a-f]{8}$/i.test(hex))throw Error('Not Single hex');const b=Buffer.alloc(4);b.writeUInt32LE(parseInt(hex,16));return b.readFloatLE();};
 const aggregate=key=>{let sum=0;for(const r of ordered)sum=Math.fround(sum+value(r[key].Q_JOD_hex));return Math.fround(sum/ordered.length);};
 const officialMean=aggregate('official'),cppMean=aggregate('cpp');if(!Object.is(officialMean,cppMean))throw Error('Video mean differs');
 fs.writeFileSync(path.join(out,'video_score.json'),JSON.stringify({scope:'one pair, local RGB48/phase0 reconstruction; not full LIVE dataset or original-submission reproduction',reference_id:referenceId,test_id:testId,complete:true,frames:indices.length,indices,all_frame_Q_and_Q_JOD_exact:true,aggregation:'ordered Single arithmetic mean of per-frame Q_JOD',official_Q_JOD:officialMean,cpp_Q_JOD:cppMean,exact:true},null,2));event('COMPLETE exact video mean Q_JOD='+cppMean);
}
main().catch(error=>{fs.writeFileSync(path.join(out,'failure.txt'),String(error.stack));event('FAILED '+error.message);process.exitCode=1;});
