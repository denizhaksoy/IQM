// Audit completed videos and count unique cases without counting reruns twice.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const [dataset,out,...roots]=process.argv.slice(2);
if(!out||!roots.length||fs.existsSync(out))throw Error('Usage: dataset fresh-report-root completed-video-roots...');
const cases=new Map(),pixels=new Set(),videos=new Map(),references=new Set();
for(const root of roots){
 const plan=read(path.join(root,'plan.json')),a=read(path.join(root,'acceptance.json')),score=read(path.join(root,'video_score.json'));
 // Legacy Daylight1 plans predate explicit IDs/CSV hash. Recover only from
 // their preserved metadata acceptance, never by changing old files.
 plan.test_id??=a.metadata.video;plan.reference_id??=a.metadata.ref;
 const csvHash=sha(path.join(dataset,'metadata/video_info_with_bitrate.csv'));
 if((plan.metadata_sha256??a.subjective_source_sha256['video_info_with_bitrate.csv'])!==csvHash||a.metadata.video!==plan.test_id||a.metadata.ref!==plan.reference_id)throw Error('Metadata differs');
 const aggregate=read(path.join(root,'aggregation_octave.json'));
 if(!a.exact_all_selected_frames||!score.complete||!score.exact||!aggregate.exact||aggregate.class!=='single'||aggregate.frames!==score.frames||score.frames!==Math.ceil(plan.decoded_frames/30)||Number(a.metadata.frm_cnt)!==plan.decoded_frames||score.cpp_Q_JOD!==score.official_Q_JOD)throw Error('Incomplete video');
 if(plan.zero_based_phase!==0||plan.stride!==30||plan.ppd!==62.729130063099447||plan.png_pixel_format!=='rgb48be'||JSON.stringify(plan.indices)!==JSON.stringify(score.indices))throw Error('Protocol differs');
 for(const [id,key] of [[plan.test_id,'test_sha256'],[plan.reference_id,'reference_sha256']])if(sha(path.join(dataset,'sample_pair',id+'.mp4'))!==plan[key])throw Error('Source video differs');
 if(plan.reference_timestamps.length!==plan.decoded_frames||plan.test_timestamps.length!==plan.decoded_frames||plan.reference_timestamps.some((t,i)=>t!==plan.test_timestamps[i]))throw Error('Alignment differs');
 let sum=0;
 const videoKey=plan.reference_id+'|'+plan.test_id;
 if(videos.has(videoKey)&&videos.get(videoKey).Q_JOD!==score.cpp_Q_JOD)throw Error('Repeated video score differs');
 videos.set(videoKey,{reference:plan.reference_id,video:plan.test_id,frames:score.frames,Q_JOD:score.cpp_Q_JOD});references.add(plan.reference_id);
 for(const [k,i] of plan.indices.entries()){
  const r=read(path.join(root,'frame'+String(i).padStart(4,'0'),'comparison.json')),key=videoKey+'|'+i;
  if(i!==k*30||r.zero_based_frame!==i||!r.exact||r.official.Q_hex!==r.cpp.Q_hex||r.official.Q_JOD_hex!==r.cpp.Q_JOD_hex||r.timestamp!==plan.reference_timestamps[i])throw Error('Frame differs');
  const decoded=path.join(root,'frame'+String(i).padStart(4,'0'),'decoded');
  if(sha(path.join(decoded,'reference.png'))!==r.reference_png_sha256||sha(path.join(decoded,'test.png'))!==r.test_png_sha256||r.official.inputs[0].source_sha256!==r.reference_png_sha256||r.official.inputs[1].source_sha256!==r.test_png_sha256)throw Error('PNG provenance differs');
  if(!/^[0-9a-f]{8}$/i.test(r.cpp.Q_JOD_hex))throw Error('Invalid Single score');const b=Buffer.alloc(4);b.writeUInt32LE(parseInt(r.cpp.Q_JOD_hex,16));const v=b.readFloatLE();if(!Number.isFinite(v))throw Error('Nonfinite score');sum=Math.fround(sum+v);
  const value={video:plan.test_id,reference:plan.reference_id,index:i,reference_png_sha256:r.reference_png_sha256,test_png_sha256:r.test_png_sha256,Q_hex:r.cpp.Q_hex,Q_JOD_hex:r.cpp.Q_JOD_hex};
  if(cases.has(key)&&JSON.stringify(cases.get(key))!==JSON.stringify(value))throw Error('Repeated frame provenance/score differs');
  cases.set(key,value);pixels.add(r.reference_png_sha256+'|'+r.test_png_sha256);
 }
 const mean=Math.fround(sum/score.frames),word=Buffer.alloc(4);word.writeFloatLE(mean);if(mean!==score.cpp_Q_JOD||aggregate.Q_JOD_hex!==word.readUInt32LE().toString(16).padStart(8,'0'))throw Error('Aggregate differs');
}
const report={scope:'audited completed local video tests only; excludes partial videos and hybrid reruns; not full dataset or original challenge reproduction',time:new Date().toISOString(),video_pairs:videos.size,reference_contents:references.size,unique_video_frame_cases:cases.size,unique_decoded_png_pairs:pixels.size,roots:roots.map(r=>path.resolve(r)),videos:[...videos.values()],frames:[...cases.values()]};
fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'evidence.json'),JSON.stringify(report,null,2));console.log(JSON.stringify({video_pairs:report.video_pairs,reference_contents:report.reference_contents,unique_video_frame_cases:report.unique_video_frame_cases,unique_decoded_png_pairs:report.unique_decoded_png_pairs}));
