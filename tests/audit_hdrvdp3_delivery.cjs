// Read-only evidence audit, writing only a new receipt. No numerical reruns.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const {fingerprintTree}=require('./live_hdr_frame_reuse.cjs');
const [manifestArg,resultsArg,snapshotArg,receiptArg]=process.argv.slice(2);
if(!receiptArg||fs.existsSync(receiptArg))throw Error('Usage frozen-manifest acceptance-root source-snapshot fresh-receipt.json');
const read=f=>JSON.parse(fs.readFileSync(f,'utf8')),sha=f=>crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex');
const m=read(manifestArg),rows=read(path.join(resultsArg,'frames.json')),checkpoint=read(path.join(resultsArg,'checkpoint.json'));
function insist(ok,msg){if(!ok)throw Error(msg);}
const hex=x=>{const b=Buffer.alloc(4);b.writeFloatLE(x);return b.readUInt32LE().toString(16).padStart(8,'0')};
insist(checkpoint.complete&&checkpoint.passed&&rows.length===219,'Incomplete acceptance');
for(const e of m.frozen_files)insist(sha(e.file)===e.sha256,'Frozen file changed: '+e.file);
insist(fingerprintTree(m.official)===m.official_tree_sha256,'Official reference changed');
const source=read(path.join(snapshotArg,'source_manifest.json'));
for(const e of source.files)insist(sha(path.join(snapshotArg,e.file))===e.sha256,'Snapshot changed: '+e.file);
const media=new Map(),videos=[];let pngChecks=0,dispatches=0;
for(const c of m.cases){const group=rows.filter(r=>r.video===c.video);insist(group.length===c.indices.length,'Missing frames '+c.video);let sr=0,si=0;
 for(let i=0;i<group.length;i++){const r=group[i];insist(r.frame===c.indices[i],'Changed sampling');const root=r.artifacts,official=read(path.join(root,'official/score.json')),iqm=read(path.join(root,'iqm/score.json')),gpu=read(path.join(root,'iqm/gpu_stages.json')),decode=read(path.join(root,'decoded/decode.json'));
  for(const [side,expected] of [['reference',c.reference_source],['test',c.test_source]]){const e=decode.records.find(x=>x.side===side);insist(e&&e.source_sha256===expected.sha256,'Decode source differs '+root);insist(sha(path.join(root,'decoded',side+'.png'))===e.png_sha256,'PNG differs '+root);insist(e.png_sha256===official.inputs[side==='reference'?0:1].source_sha256,'Official input differs');insist(e.png_sha256===r[side==='reference'?'reference_png_sha256':'test_png_sha256'],'Comparison input differs');pngChecks++;media.set(expected.file,expected.sha256);}
  for(const key of ['Q','Q_JOD'])insist(hex(Number(official[key+'_text']))===official[key+'_hex']&&hex(Number(iqm[key+'_text']))===iqm[key+'_hex'],'Text/bit mismatch '+root);
  for(const key of ['Q_hex','Q_JOD_hex'])insist(official[key]===iqm[key]&&official[key]===r.official[key]&&iqm[key]===r.iqm[key],'Score mismatch '+root);
  insist(official.ppd===m.ppd&&iqm.ppd===m.ppd&&official.fftw_threads===3&&iqm.fftw_threads===3&&official.fftw_planner==='estimate'&&iqm.fftw_planner==='estimate'&&official.reference_version==='3.0.7'&&official.backend==='CPU'&&official.input_precision==='single','Settings differ');
  for(const [k,v] of Object.entries({matrix_dispatches:4,comparison_dispatches:1,mutual_dispatches:6,comparison_resize_calls:6,comparison_resize_dispatches:8,adaptation_input_dispatches:2,padding_dispatches:8}))insist(gpu[k]===v,'GPU count differs '+root);
  insist(!/VUID-|Validation Error|\[ERROR\]/.test(fs.readFileSync(path.join(root,'iqm.log'),'utf8')),'Validation log error');
  dispatches+=gpu.matrix_dispatches+gpu.comparison_dispatches+gpu.mutual_dispatches+gpu.comparison_resize_dispatches+gpu.adaptation_input_dispatches+gpu.padding_dispatches;
  sr=Math.fround(sr+Number(official.Q_JOD_text));si=Math.fround(si+Number(iqm.Q_JOD_text));
 }
 const meanR=hex(Math.fround(sr/group.length)),meanI=hex(Math.fround(si/group.length)),saved=read(path.join(resultsArg,c.video,'video_score.json'));insist(meanR===meanI&&meanR===saved.official_mean_hex&&meanI===saved.iqm_mean_hex,'Mean differs');videos.push({video:c.video,frames:group.length,mean_hex:meanR});
}
for(const [file,hash] of media)insist(sha(file)===hash,'Current media differs');
const receipt={passed:true,date:new Date().toISOString(),manifest_sha256:sha(manifestArg),source_files:source.files.length,frozen_files:m.frozen_files.length,source_media:media.size,verified_png_files:pngChecks,exact_frames:rows.length,exact_video_means:videos.length,recorded_gpu_dispatches:dispatches,vulkan_validation_errors:0,videos,known_issues:checkpoint.known_issues,scope:'post-run evidence/hash audit, not numerical rerun, kernel timing or universal parity'};
fs.writeFileSync(receiptArg,JSON.stringify(receipt,null,2));console.log(JSON.stringify(receipt));
