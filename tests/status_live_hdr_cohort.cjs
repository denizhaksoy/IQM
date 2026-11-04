// Read-only progress snapshot. Partial frame results are never video acceptance.
const fs=require('node:fs'),path=require('node:path');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const [file]=process.argv.slice(2);if(!file)throw Error('Usage: scheduler-config.json');
const config=read(file),base=path.dirname(path.resolve(file)),out=path.resolve(base,config.output),cohort=read(path.resolve(base,config.cohort)),state=read(path.join(out,'checkpoint.json'));
const videos=state.videos.map(v=>{const score=read(path.resolve(out,v.root,'video_score.json'));return {video:v.video,frames:score.frames,exact:score.exact,Q_JOD:score.cpp_Q_JOD};});
const attempt=state.attempts.at(-1);let active=null;
if(attempt&&!videos.some(v=>v.video===attempt.video)){
 const root=path.resolve(out,attempt.root),planFile=path.join(root,'plan.json'),plan=fs.existsSync(planFile)?read(planFile):null;
 const exact=plan?plan.indices.filter(i=>{const f=path.join(root,'frame'+String(i).padStart(4,'0'),'comparison.json');if(!fs.existsSync(f))return false;const r=read(f);return r.exact&&r.official.Q_hex===r.cpp.Q_hex&&r.official.Q_JOD_hex===r.cpp.Q_JOD_hex;}):[];
 const failure=path.join(root,'failure.txt');active={video:attempt.video,selected_frames:plan?.indices.length??null,partial_exact_frames:exact.length,accepted:false,failure:fs.existsSync(failure)?fs.readFileSync(failure,'utf8'):null};
}
const lock=path.join(out,'scheduler.lock');let scheduler=null;
if(fs.existsSync(lock)){scheduler=read(lock);try{process.kill(scheduler.pid,0);scheduler.process_exists=true;}catch{scheduler.process_exists=false;}}
console.log(JSON.stringify({scope:'progress only; local declared cohort, not full LIVE dataset or original submission',time:new Date().toISOString(),accepted_videos:videos.length,total_videos:cohort.cases.length,accepted_frames:videos.reduce((n,v)=>n+v.frames,0),videos,active,scheduler},null,2));
