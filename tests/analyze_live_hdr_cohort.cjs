// Explicit local analysis: complete declared cohort, video-level JOD vs bright MOS.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
function finite(v){if(!Array.isArray(v)||v.length<3||v.some(x=>!Number.isFinite(x)))throw Error('At least three finite observations required');}
function sum(values){let s=0,c=0;for(const x of values){const next=s+x;c+=Math.abs(s)>=Math.abs(x)?(s-next)+x:(x-next)+s;s=next;}return s+c;}
function pearson(x,y){finite(x);finite(y);if(x.length!==y.length)throw Error('Paired sample count differs');const mx=sum(x)/x.length,my=sum(y)/y.length,dx=x.map(v=>v-mx),dy=y.map(v=>v-my);const xx=sum(dx.map(v=>v*v)),yy=sum(dy.map(v=>v*v));if(xx===0||yy===0)throw Error('Correlation undefined for a constant vector');return Math.max(-1,Math.min(1,sum(dx.map((v,i)=>v*dy[i]))/Math.sqrt(xx*yy)));}
function ranks(v){finite(v);const order=v.map((value,index)=>({value,index})).sort((a,b)=>a.value-b.value),out=Array(v.length);for(let i=0;i<order.length;){let j=i+1;while(j<order.length&&order[j].value===order[i].value)++j;const rank=(i+1+j)/2;for(let k=i;k<j;++k)out[order[k].index]=rank;i=j;}return out;}
function spearman(x,y){return pearson(ranks(x),ranks(y));}
module.exports={pearson,spearman,ranks};
function main(){
 const [cohortPath,predictionsPath,out,ack]=process.argv.slice(2);
 if(!out||ack!=='--local-cohort'||fs.existsSync(out))throw Error('Usage: cohort.json video-predictions.json fresh-root --local-cohort');
 const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
 const cohort=read(cohortPath),predictions=read(predictionsPath);
 if(cohort.target!=='bright_mos'||cohort.recalibration!==false||predictions.format!=='IQM_LIVE_HDR_VIDEO_PREDICTIONS_1')throw Error('Explicit analysis policy mismatch');
 const cases=cohort.cases,videos=predictions.videos;
 if(cases.length<3||videos.length!==cases.length||new Set(cases.map(c=>c.video)).size!==cases.length||new Set(videos.map(c=>c.video)).size!==videos.length)throw Error('Complete unique cohort predictions required; no partial-cohort analysis');
 const expected=new Set(cases.map(c=>c.video));if(videos.some(v=>!expected.has(v.video)))throw Error('Prediction cohort differs');
 const rows=[];
 for(const c of cases){if(c.video===c.reference||cohort.excluded_references.includes(c.video))throw Error('Reference video included');
  const p=videos.find(v=>v.video===c.video),base=path.resolve(path.dirname(predictionsPath),p.root);
  const accepted=read(path.join(base,'acceptance.json')),score=read(path.join(base,'video_score.json')),aggregate=read(path.join(base,'aggregation_octave.json')),plan=read(path.join(base,'plan.json'));
  if(plan.test_id!==c.video||plan.reference_id!==c.reference||plan.ppd!==62.729130063099447||plan.zero_based_phase!==0||plan.stride!==30||plan.png_pixel_format!=='rgb48be'||score.frames!==Math.ceil(plan.decoded_frames/30))throw Error('Declared local sampling/viewing protocol differs: '+c.video);
  if(!accepted.exact_all_selected_frames||!score.complete||!score.exact||!aggregate.exact||aggregate.frames!==score.frames||accepted.metadata.video!==c.video||accepted.metadata.ref!==c.reference||score.official_Q_JOD!==score.cpp_Q_JOD)throw Error('Incomplete or mismatched video acceptance: '+c.video);
  const word=Buffer.alloc(4);word.writeFloatLE(score.cpp_Q_JOD);const meanHex=word.readUInt32LE().toString(16).padStart(8,'0');
  if(aggregate.class!=='single'||aggregate.Q_JOD_hex!==meanHex||Math.fround(score.cpp_Q_JOD)!==score.cpp_Q_JOD)throw Error('Single video aggregation differs: '+c.video);
  for(const [name,hash] of Object.entries(cohort.source_csv_sha256))if(accepted.subjective_source_sha256[name]!==hash)throw Error('Subjective metadata version differs');
  const mos=Number(c.bright.bright_mos);if(typeof c.bright.bright_mos!=='string'||!c.bright.bright_mos.trim()||c.bright.bright_mos!==accepted.subjective_labels.bright.bright_mos||!Number.isFinite(mos)||!Number.isFinite(score.cpp_Q_JOD))throw Error('MOS/prediction mismatch');
  rows.push({video:c.video,reference:c.reference,content:c.content,frames:score.frames,reference_runs_per_frame:plan.reference_runs??1,mos,official:score.official_Q_JOD,cpp:score.cpp_Q_JOD,acceptance_sha256:sha(path.join(base,'acceptance.json'))});
 }
 const y=rows.map(r=>r.mos),ref=rows.map(r=>r.official),cpp=rows.map(r=>r.cpp),metrics=x=>({SROCC:spearman(x,y),raw_PLCC:pearson(x,y)});
 const result={scope:'complete declared local cohort; not original challenge reproduction',videos:rows.length,contents:new Set(rows.map(r=>r.content)).size,author_cohort_verified:cohort.original_train_video_info_csv_verified===true,target:'bright_mos',prediction:'native video mean Q_JOD',rank_ties:'average',fitted_mapping:false,retraining:false,RMSE:null,RMSE_reason:'No documented JOD-to-MOS mapping supplied; different units are not silently fitted',official:metrics(ref),cpp:metrics(cpp),cohort_sha256:sha(cohortPath),prediction_manifest_sha256:sha(predictionsPath),rows};
 result.unresolved_reference_discrepancies=predictions.unresolved_reference_discrepancies??[];
 if(!Array.isArray(result.unresolved_reference_discrepancies))throw Error('Reference discrepancy metadata must be an array');
 result.historical_reference_discrepancy_review=result.unresolved_reference_discrepancies.length?'unresolved':'no_issues_reported_in_manifest';
 fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'analysis.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify({videos:result.videos,official:result.official,cpp:result.cpp,unresolved_reference_discrepancies:result.unresolved_reference_discrepancies.length}));
}
if(require.main===module)main();
