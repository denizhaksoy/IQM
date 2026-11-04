// Integrity fixtures, not decoded LIVE images or numerical metric validation.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict');
const {reuseFrame,fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh test root required');
const write=(p,v)=>{fs.mkdirSync(path.dirname(p),{recursive:true});fs.writeFileSync(p,JSON.stringify(v));},sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const seed=path.join(out,'seed'),frame=path.join(seed,'frame0000'),ref=path.join(out,'new-reference.png'),test=path.join(out,'new-test.png');
fs.mkdirSync(path.join(frame,'decoded'),{recursive:true});fs.writeFileSync(ref,'reference fixture');fs.writeFileSync(test,'test fixture');fs.copyFileSync(ref,path.join(frame,'decoded/reference.png'));fs.copyFileSync(test,path.join(frame,'decoded/test.png'));
const plan={calculation_sha256:'calculation fixture',reference_id:'ref',test_id:'dis',reference_sha256:'refvideo',test_sha256:'disvideo',metadata_sha256:'metadata',executable_sha256:'exe',fftw_sha256:'dll',decoded_frames:1,ppd:62.729130063099447,zero_based_phase:0,stride:30,png_pixel_format:'rgb48be',indices:[0],reference_timestamps:['0'],test_timestamps:['0']};
const r={exact:true,zero_based_frame:0,timestamp:'0',reference_png_sha256:sha(ref),test_png_sha256:sha(test),official:{reference_version:'3.0.7',backend:'CPU',input_precision:'single',ppd:plan.ppd,Q_hex:'3f800000',Q_JOD_hex:'40000000',inputs:[{source_sha256:sha(ref)},{source_sha256:sha(test)}]},cpp:{Q_hex:'3f800000',Q_JOD_hex:'40000000'}};
const planFile=path.join(seed,'plan.json'),comparison=path.join(frame,'comparison.json');write(planFile,plan);write(comparison,r);
const checks=[];assert.equal(reuseFrame(seed,plan,0,ref,test).source_comparison_sha256,sha(comparison));checks.push('exact matched seed reused');
for(const key of ['calculation_sha256','executable_sha256','reference_sha256','png_pixel_format']){assert.equal(reuseFrame(seed,{...plan,[key]:'changed'},0,ref,test),null);checks.push(key+' change recomputes');}
write(planFile,{...plan,calculation_sha256:undefined});assert.equal(reuseFrame(seed,plan,0,ref,test),null);write(planFile,plan);checks.push('legacy fingerprint missing recomputes');
fs.writeFileSync(test,'different new decode');assert.equal(reuseFrame(seed,plan,0,ref,test),null);fs.copyFileSync(path.join(frame,'decoded/test.png'),test);checks.push('new decode mismatch recomputes');
write(comparison,{...r,exact:false});assert.equal(reuseFrame(seed,plan,0,ref,test),null);write(comparison,r);checks.push('failed prior frame not reused');
const bad=structuredClone(r);bad.official.inputs[0].source_sha256='corrupt';write(comparison,bad);assert.throws(()=>reuseFrame(seed,plan,0,ref,test),/provenance/);write(comparison,r);checks.push('corrupt source provenance rejected');
fs.writeFileSync(path.join(frame,'decoded/test.png'),'corrupt prior image');assert.throws(()=>reuseFrame(seed,plan,0,ref,test),/provenance/);checks.push('corrupt prior PNG rejected');
const tree=path.join(out,'tree');fs.mkdirSync(tree);fs.writeFileSync(path.join(tree,'model.m'),'v1');const before=fingerprintTree(tree);fs.writeFileSync(path.join(tree,'model.m'),'v2');assert.notEqual(fingerprintTree(tree),before);checks.push('reference code change alters fingerprint');
write(path.join(out,'checks.json'),{scope:'engineering reuse integrity only',passed:checks});console.log(JSON.stringify(checks));
