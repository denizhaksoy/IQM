// Read-only acceptance audit: reconstruct every video mean from frame bits and
// verify source/runtime hashes before computing unfitted bright-MOS correlations.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const {pearson,spearman}=require('./analyze_live_hdr_cohort.cjs');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function single(hex){if(!/^[0-9a-f]{8}$/i.test(hex))throw Error('Invalid Single word');const b=Buffer.alloc(4);b.writeUInt32LE(parseInt(hex,16));const x=b.readFloatLE();if(!Number.isFinite(x))throw Error('Nonfinite score');return x;}
function auditVideo(root,c){
 const plan=read(path.join(root,'plan.json')),score=read(path.join(root,'video_score.json'));
 if(plan.test_id!==c.video||plan.reference_id!==c.reference||plan.zero_based_phase!==0||plan.stride!==30||plan.png_pixel_format!=='rgb48be'||plan.ppd!==62.729130063099447||plan.decoded_frames!==c.metadata_frames||!score.complete||!score.exact)throw Error('Protocol/incomplete video '+c.video);
 const indices=Array.from({length:Math.ceil(c.metadata_frames/30)},(_,i)=>30*i);
 if(JSON.stringify(plan.indices)!==JSON.stringify(indices)||JSON.stringify(score.indices)!==JSON.stringify(indices)||score.frames!==indices.length)throw Error('Sampling differs');
 if(plan.reference_timestamps.length!==c.metadata_frames||plan.test_timestamps.length!==c.metadata_frames)throw Error('Timing count differs');
 if(plan.calculation.alignment_policy==='zero-based-index/exact-relative-CFR-clock-grid'){
  const rt=read(path.join(root,'reference.timing.json')),tt=read(path.join(root,'test.timing.json')),certificate=require('./live_hdr_clock_grid.cjs').verifyClockPair(rt,tt,c.metadata_frames);
  if(JSON.stringify(certificate)!==JSON.stringify(plan.alignment)||JSON.stringify(certificate)!==JSON.stringify(read(path.join(root,'alignment.json')))||JSON.stringify(rt.frames.map(f=>f.best_effort_timestamp_time))!==JSON.stringify(plan.reference_timestamps)||JSON.stringify(tt.frames.map(f=>f.best_effort_timestamp_time))!==JSON.stringify(plan.test_timestamps))throw Error('Clock-grid certificate differs');
 }else if(plan.reference_timestamps.some((t,i)=>t!==plan.test_timestamps[i]))throw Error('Legacy timing differs');
 for(const [file,hash] of [[plan.reference,plan.reference_sha256],[plan.test,plan.test_sha256]])if(sha(file)!==hash)throw Error('Video changed');
 for(const entry of plan.calculation.runtime_hashes)if(sha(entry.file)!==entry.sha256)throw Error('Runtime changed');
 let official=0,cpp=0;
 for(const i of indices){const frame=path.join(root,'frame'+String(i).padStart(4,'0')),record=read(path.join(frame,'comparison.json')),a=read(path.join(frame,'official/score.json')),b=read(path.join(frame,'cpp/score.json')),stages=read(path.join(frame,'cpp/gpu_stages.json'));
  if(!record.exact||record.zero_based_frame!==i||record.timestamp!==plan.reference_timestamps[i]||a.Q_hex!==b.Q_hex||a.Q_JOD_hex!==b.Q_JOD_hex||record.official.Q_hex!==a.Q_hex||record.official.Q_JOD_hex!==a.Q_JOD_hex||record.cpp.Q_hex!==b.Q_hex||record.cpp.Q_JOD_hex!==b.Q_JOD_hex)throw Error('Frame scores differ '+c.video+'/'+i);
  if(a.reference_version!=='3.0.7'||a.input_precision!=='single'||a.ppd!==plan.ppd||a.fftw_threads!==3||a.fftw_planner!=='estimate'||a.backend!=='CPU'||a.persistent_linear_exports!==false)throw Error('Reference profile differs');
  for(const [j,name] of ['reference','test'].entries()){const hash=sha(path.join(frame,'decoded',name+'.png'));if(hash!==a.inputs[j].source_sha256||hash!==record[name+'_png_sha256'])throw Error('PNG changed');}
  if(stages.matrix_dispatches!==4||stages.comparison_dispatches!==1||stages.mutual_dispatches!==6||stages.comparison_resize_calls!==6||stages.comparison_resize_dispatches!==8||stages.full_gpu!==false)throw Error('GPU stages differ');
  if(/VUID-|Validation Error|\[ERROR\]/i.test(fs.readFileSync(path.join(frame,'cpp_runner.log'),'utf8')))throw Error('Vulkan error');
  official=Math.fround(official+single(a.Q_JOD_hex));cpp=Math.fround(cpp+single(b.Q_JOD_hex));
 }
 official=Math.fround(official/indices.length);cpp=Math.fround(cpp/indices.length);
 if(!Object.is(official,cpp)||!Object.is(official,score.official_Q_JOD)||!Object.is(cpp,score.cpp_Q_JOD))throw Error('Video mean differs');
 return {video:c.video,reference:c.reference,frames:indices.length,official,cpp,mos:Number(c.bright.bright_mos)};
}
function main(){
 const [cohortFile,root,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: full-cohort.json completed-controller-root fresh-audit.json');
 const cohort=read(cohortFile),checkpoint=read(path.join(root,'checkpoint.json'));
 if(!checkpoint.complete||!checkpoint.passed||cohort.cases.length!==279)throw Error('Full cohort not complete; no partial-correlation claim');
 const videos=read(path.join(root,'accepted_videos.json'));if(videos.length!==279||new Set(videos.map(v=>v.video)).size!==279)throw Error('Missing/duplicate videos');
 const rows=cohort.cases.map(c=>{const v=videos.find(v=>v.video===c.video);if(!v||v.reference!==c.reference)throw Error('Pairing differs');return auditVideo(v.result_root,c);});
 if(rows.some(r=>!Number.isFinite(r.mos)))throw Error('MOS missing');const y=rows.map(r=>r.mos),metrics=k=>({raw_PLCC:pearson(rows.map(r=>r[k]),y),SROCC:spearman(rows.map(r=>r[k]),y)});
 fs.writeFileSync(out,JSON.stringify({complete:true,passed:true,videos:rows.length,frames:rows.reduce((n,r)=>n+r.frames,0),scope:'full supplied archive; not author-confirmed paper split',official:metrics('official'),cpp:metrics('cpp'),fitted_mapping:false,recalibration:false,original_paper_reproduction_verified:false,historical_reference_discrepancy_resolved:false,rows},null,2));
}
module.exports={auditVideo};if(require.main===module)main();
