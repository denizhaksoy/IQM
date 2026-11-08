const assert=require('node:assert/strict'),{verifyClockPair}=require('./live_hdr_clock_grid.cjs');
function make(den,start,count=10){return {streams:[{avg_frame_rate:'60000/1001',r_frame_rate:'60000/1001',time_base:'1/'+den,start_pts:start,nb_frames:String(count)}],frames:Array.from({length:count},(_,i)=>({best_effort_timestamp:start+Math.floor(i*1001*den/60000)}))};}
const r=make(90000,16200),t=make(60000,0);assert.equal(verifyClockPair(r,t,10).reference.quantization_modes.includes('floor'),true);
const missing=structuredClone(t);missing.frames.splice(4,1);assert.throws(()=>verifyClockPair(r,missing,10),/frame count/);
const shifted=structuredClone(t);shifted.frames[5].best_effort_timestamp+=1001;assert.throws(()=>verifyClockPair(r,shifted,10),/clock grid/);
const jitter=structuredClone(t);jitter.frames[5].best_effort_timestamp+=1;assert.throws(()=>verifyClockPair(r,jitter,10),/clock grid/);
const wrongRate=structuredClone(t);wrongRate.streams[0].avg_frame_rate='60/1';assert.throws(()=>verifyClockPair(r,wrongRate,10),/frame rate/);
console.log('Clock-grid tests passed: exact offset/timebases accepted; dropped frame, whole-frame shift, one-tick jitter and rate inconsistency rejected');
