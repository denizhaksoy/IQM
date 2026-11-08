// Gate full supplied-archive evaluation on the compact pilot matching preserved
// Daylight frame scores, source PNG hashes and the ordered Single video mean.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve('validation/live_hdr_full_scores_20261108_run01'),pilot=path.resolve('validation/live_hdr_compact_daylight_20261107_run02');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const pause=()=>new Promise(r=>setTimeout(r,30000));
async function main(){
 if(fs.existsSync(root))throw Error('Fresh output required');fs.mkdirSync(root);fs.copyFileSync(__filename,path.join(root,'controller_snapshot.cjs'));
 const save=x=>fs.writeFileSync(path.join(root,'checkpoint.json'),JSON.stringify({time:new Date().toISOString(),...x},null,2));
 save({stage:'waiting_for_pilot',complete:false});
 while(!fs.existsSync(path.join(pilot,'video_score.json'))){if(fs.existsSync(path.join(pilot,'failure.txt')))throw Error('Pilot failed; inspect preserved results');await pause();}
 const score=read(path.join(pilot,'video_score.json')),old=path.resolve('validation/live_hdr_video_20261101_run01');
 if(!score.complete||!score.exact||score.frames!==16||Math.fround(score.cpp_Q_JOD)!==Math.fround(read(path.join(old,'video_score.json')).cpp_Q_JOD))throw Error('Pilot video mean differs');
 for(const index of score.indices){const name='frame'+String(index).padStart(4,'0'),a=read(path.join(pilot,name,'comparison.json')),b=read(path.join(old,name,'comparison.json'));if(!a.exact||a.official.Q_hex!==b.official.Q_hex||a.official.Q_JOD_hex!==b.official.Q_JOD_hex||a.reference_png_sha256!==b.reference_png_sha256||a.test_png_sha256!==b.test_png_sha256)throw Error('Pilot preserved frame differs '+name);}
 fs.writeFileSync(path.join(root,'pilot_acceptance.json'),JSON.stringify({passed:true,frames:16,video_mean_exact:true,source_png_hashes_exact:true,pilot,scope:'local workflow equivalence, not paper cohort confirmation'},null,2));
 const cohort=read('validation/live_hdr_full_archive_20261106_run01/full_archive_cohort.json'),inventory=read('validation/live_hdr_full_archive_staging_20261107_run01/inventory.json');
 if(inventory.videos!==310||inventory.distorted_pairs!==279||inventory.cohort_sha256!==sha('validation/live_hdr_full_archive_20261106_run01/full_archive_cohort.json'))throw Error('Full staging gate failed');
 const previous=read('validation/live_hdr_full_scores_20261107_run03/accepted_videos.json');
 const {auditVideo}=require('./audit_live_hdr_full_clockgrid.cjs');
 const alignmentPilot=path.resolve('validation/live_hdr_light2_clockgrid_20261107_run01');
 save({stage:'waiting_for_clockgrid_pilot',complete:false});
 while(!fs.existsSync(path.join(alignmentPilot,'video_score.json'))){if(fs.existsSync(path.join(alignmentPilot,'failure.txt')))throw Error('Clock-grid pilot failed');await pause();}
 const alignmentCase=cohort.cases.find(c=>c.video==='1080p_1M_Light2');auditVideo(alignmentPilot,alignmentCase);
 const dependencies=['tests/live_hdr_clock_grid.cjs','tests/audit_live_hdr_full_clockgrid.cjs','tests/decode_live_hdr_frame.cjs','tests/score_hdrvdp3_compact_reference.m'].map(file=>({file,sha256:sha(file)}));fs.writeFileSync(path.join(root,'dependency_hashes.json'),JSON.stringify(dependencies,null,2));
 const runner='tests/run_live_hdr_compact_video_clockgrid.cjs',runnerHash=sha(runner),results=[];fs.copyFileSync(runner,path.join(root,'runner_snapshot.cjs'));
 const blocked=[{video:'1080p_1M_Light3',reason:'Reference has 362 decoded frames versus 367 header/metadata; final PTS gaps. No alignment assumption or score computed.',evidence:'validation/live_hdr_full_scores_20261107_run03/1080p_1M_Light3'}];
 fs.writeFileSync(path.join(root,'blocked_videos.json'),JSON.stringify(blocked,null,2));
 for(const c of cohort.cases){
  if(blocked.some(b=>b.video===c.video))continue;
  if(sha(runner)!==runnerHash||dependencies.some(e=>sha(e.file)!==e.sha256))throw Error('Evaluation dependency modified');
  const preserved=previous.find(r=>r.video===c.video);
  if(preserved)auditVideo(preserved.result_root,c);
  const destination=preserved?preserved.result_root:(c.video==='1080p_1M_Light2'?alignmentPilot:(c.video==='4k_3M_Daylight1'?pilot:path.join(root,c.video)));
  save({stage:'evaluating',complete:false,completed_videos:results.length,total_videos:279,active_video:c.video});
  if(!preserved&&destination!==pilot&&destination!==alignmentPilot)await new Promise((resolve,reject)=>{const child=cp.spawn(process.execPath,[runner,'validation/live_hdr_639290787607997134','validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7','validation/comparison_resize_runtime_20261106_run01',destination,c.video,c.reference],{windowsHide:true});const log=path.join(root,c.video+'.log');child.stdout.on('data',b=>fs.appendFileSync(log,b));child.stderr.on('data',b=>fs.appendFileSync(log,b));child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error(c.video+' exited '+code)));});
  const value=read(path.join(destination,'video_score.json'));if(!value.complete||!value.exact||value.frames!==Math.ceil(c.metadata_frames/30))throw Error('Incomplete video '+c.video);
  results.push({video:c.video,reference:c.reference,result_root:destination,score:value});fs.writeFileSync(path.join(root,'accepted_videos.json'),JSON.stringify(results,null,2));
 }
 save({stage:'eligible_subset_complete',complete:false,passed:false,eligible_subset_complete:true,blocked_videos:blocked.length,videos:results.length,frames:results.reduce((n,r)=>n+r.score.frames,0),original_paper_reproduction_verified:false,historical_reference_discrepancy_resolved:false});
}
main().catch(e=>{if(fs.existsSync(root)){fs.writeFileSync(path.join(root,'failure.txt'),e.stack);fs.writeFileSync(path.join(root,'status_failed.json'),JSON.stringify({failed:true,error:e.message}));}console.error(e.stack);process.exitCode=1;});
