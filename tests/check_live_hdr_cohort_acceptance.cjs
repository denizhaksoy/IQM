// Engineering fixtures for scheduler integrity guards; not LIVE numerical results.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict');
const {accepted}=require('./run_live_hdr_cohort.cjs');
const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh test output required');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const write=(p,v)=>{fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,JSON.stringify(v));};
const dataset=path.join(out,'dataset'),root=path.join(out,'video'),c={video:'distortion',reference:'reference',metadata_frames:31};
for(const file of ['metadata/video_info_with_bitrate.csv','sample_pair/distortion.mp4','sample_pair/reference.mp4']){const p=path.join(dataset,file);fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,'engineering fixture '+file);}
const plan={test_id:c.video,reference_id:c.reference,decoded_frames:31,indices:[0,30],zero_based_phase:0,stride:30,ppd:62.729130063099447,png_pixel_format:'rgb48be',metadata_sha256:sha(path.join(dataset,'metadata/video_info_with_bitrate.csv')),test_sha256:sha(path.join(dataset,'sample_pair/distortion.mp4')),reference_sha256:sha(path.join(dataset,'sample_pair/reference.mp4')),reference_timestamps:Array.from({length:31},(_,i)=>String(i)),test_timestamps:Array.from({length:31},(_,i)=>String(i))};
const score={complete:true,exact:true,frames:2,indices:[0,30],official_Q_JOD:1.5,cpp_Q_JOD:1.5};
write(path.join(root,'plan.json'),plan);write(path.join(root,'video_score.json'),score);write(path.join(root,'acceptance.json'),{exact_all_selected_frames:true});write(path.join(root,'aggregation_octave.json'),{exact:true,class:'single',frames:2,Q_JOD_hex:'3fc00000'});
for(const [i,hex] of [[0,'3f800000'],[30,'40000000']]){const frame=path.join(root,'frame'+String(i).padStart(4,'0'));for(const name of ['reference.png','test.png']){const p=path.join(frame,'decoded',name);fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,'fixture '+i+name);}const ref=sha(path.join(frame,'decoded/reference.png')),test=sha(path.join(frame,'decoded/test.png'));write(path.join(frame,'comparison.json'),{exact:true,zero_based_frame:i,timestamp:String(i),reference_png_sha256:ref,test_png_sha256:test,official:{Q_hex:'3f800000',Q_JOD_hex:hex,inputs:[{source_sha256:ref},{source_sha256:test}]},cpp:{Q_hex:'3f800000',Q_JOD_hex:hex}});}
accepted(root,c,dataset);const checks=['valid engineering fixture'];
function reject(file,mutate,label,pattern){const p=path.join(root,file),original=fs.readFileSync(p),value=JSON.parse(original);mutate(value);write(p,value);try{assert.throws(()=>accepted(root,c,dataset),pattern);checks.push(label);}finally{fs.writeFileSync(p,original);}}
reject('plan.json',p=>p.test_timestamps[5]='wrong','decoded alignment',/alignment/);
reject('plan.json',p=>p.decoded_frames=30,'metadata frame count',/count/);
reject('video_score.json',s=>s.indices=[0,29],'sample indices',/count/);
reject('frame0000/comparison.json',r=>r.official.inputs[0].source_sha256='wrong','reference input provenance',/provenance/);
reject('frame0000/comparison.json',r=>{r.cpp.Q_JOD_hex='40000000';r.official.Q_JOD_hex='40000000';},'frame-to-video mean',/mean/);
reject('frame0000/comparison.json',r=>r.cpp.Q_hex='00000000','exact Q requirement',/result/);
const png=path.join(root,'frame0000/decoded/test.png'),original=fs.readFileSync(png);fs.writeFileSync(png,'mutated');assert.throws(()=>accepted(root,c,dataset),/input changed/);fs.writeFileSync(png,original);checks.push('decoded PNG hash');
write(path.join(out,'checks.json'),{scope:'engineering integrity fixtures only',passed:checks});console.log(JSON.stringify(checks));
