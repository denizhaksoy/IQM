const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const {pearson,spearman,ranks}=require('./analyze_live_hdr_cohort.cjs');
const [oraclePath,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: octave-oracle.json fresh-root');
const oracle=JSON.parse(fs.readFileSync(oraclePath,'utf8'));
const x=[1,2,2,4,5,5],y=[6,3,5,2,2,1],actual={pearson:pearson(x,y),spearman:spearman(x,y)};
for(const key of Object.keys(actual))if(Math.abs(actual[key]-oracle[key])>1e-14)throw Error('Octave statistics comparison failed');
if(JSON.stringify(ranks(x))!==JSON.stringify([1,2.5,2.5,4,5.5,5.5]))throw Error('Average tie ranks differ');
let rejected=0;for(const values of [[1,1,1],[1,NaN,3]])try{pearson(values,[1,2,3]);}catch{++rejected;}if(rejected!==2)throw Error('Undefined input accepted');
fs.mkdirSync(out,{recursive:true});const cohort=path.resolve('validation/live_hdr_public_candidate_20261101_run01/candidate_cohort.json'),input=path.join(out,'incomplete.json'),blocked=path.join(out,'must-not-exist');
fs.writeFileSync(input,JSON.stringify({format:'IQM_LIVE_HDR_VIDEO_PREDICTIONS_1',videos:[]}));
const run=cp.spawnSync(process.execPath,['tests/analyze_live_hdr_cohort.cjs',cohort,input,blocked,'--local-cohort'],{encoding:'utf8',windowsHide:true});
if(run.status===0||fs.existsSync(blocked)||!run.stderr.includes('Complete unique cohort predictions required'))throw Error('Incomplete cohort was not rejected');
// Synthetic accepted-record fixtures exercise file joins and CLI policy only.
const fixture={scope:'ENGINEERING FIXTURE, NOT DATASET RESULTS',target:'bright_mos',recalibration:false,original_train_video_info_csv_verified:false,excluded_references:['ref0','ref1','ref2'],source_csv_sha256:{fixture:'0'.repeat(64)},cases:[]},manifest={format:'IQM_LIVE_HDR_VIDEO_PREDICTIONS_1',videos:[],unresolved_reference_discrepancies:[{scope:'engineering-only',status:'unresolved_fixture'}]};
for(let i=0;i<x.length;++i){const video='engineering'+i,reference='ref'+Math.floor(i/2),base=path.join(out,video);fs.mkdirSync(base);const bright={bright_mos:String(y[i])};fixture.cases.push({video,reference,content:reference,bright});manifest.videos.push({video,root:video});
 fs.writeFileSync(path.join(base,'acceptance.json'),JSON.stringify({scope:'ENGINEERING FIXTURE ONLY',exact_all_selected_frames:true,metadata:{video,ref:reference},subjective_source_sha256:fixture.source_csv_sha256,subjective_labels:{bright}}));
 fs.writeFileSync(path.join(base,'video_score.json'),JSON.stringify({scope:'ENGINEERING FIXTURE ONLY',complete:true,exact:true,frames:3,official_Q_JOD:x[i],cpp_Q_JOD:x[i]}));
 const meanWord=Buffer.alloc(4);meanWord.writeFloatLE(x[i]);fs.writeFileSync(path.join(base,'aggregation_octave.json'),JSON.stringify({scope:'ENGINEERING FIXTURE ONLY',exact:true,frames:3,class:'single',Q_JOD_hex:meanWord.readUInt32LE().toString(16).padStart(8,'0')}));
 fs.writeFileSync(path.join(base,'plan.json'),JSON.stringify({test_id:video,reference_id:reference,ppd:62.729130063099447,zero_based_phase:0,stride:30,png_pixel_format:'rgb48be',decoded_frames:90}));
}
const fixturePath=path.join(out,'engineering-cohort.json'),manifestPath=path.join(out,'engineering-predictions.json'),acceptedOut=path.join(out,'engineering-analysis');fs.writeFileSync(fixturePath,JSON.stringify(fixture));fs.writeFileSync(manifestPath,JSON.stringify(manifest));
const positive=cp.spawnSync(process.execPath,['tests/analyze_live_hdr_cohort.cjs',fixturePath,manifestPath,acceptedOut,'--local-cohort'],{encoding:'utf8',windowsHide:true});if(positive.status!==0)throw Error('Complete engineering fixture failed: '+positive.stderr);
const report=JSON.parse(fs.readFileSync(path.join(acceptedOut,'analysis.json'),'utf8'));if(report.videos!==6||report.contents!==3||report.RMSE!==null||report.fitted_mapping||report.author_cohort_verified)throw Error('Analysis scope/policy differs');
if(Math.abs(report.cpp.SROCC-oracle.spearman)>1e-14||Math.abs(report.cpp.raw_PLCC-oracle.pearson)>1e-14||JSON.stringify(report.cpp)!==JSON.stringify(report.official))throw Error('Complete CLI statistics differ');
if(report.historical_reference_discrepancy_review!=='unresolved'||JSON.stringify(report.unresolved_reference_discrepancies)!==JSON.stringify(manifest.unresolved_reference_discrepancies)||report.rows.some(r=>r.reference_runs_per_frame!==1))throw Error('Reference history/repeat policy was lost');
fs.writeFileSync(path.join(out,'checks.json'),JSON.stringify({scope:'engineering statistics checks, not LIVE subjective benchmark results',passed:true,actual,oracle,maximum_absolute_tolerance:1e-14,ties_verified:true,undefined_inputs_rejected:rejected,incomplete_cohort_rejected:true,complete_engineering_fixture_verified:true},null,2));console.log('Tie/undefined-input/cohort guards, complete engineering CLI and Octave statistics checks passed');
