// Conservative reuse from an interrupted run, after independently decoding again.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function fingerprintTree(root){
 const entries=[];function visit(dir){for(const entry of fs.readdirSync(dir,{withFileTypes:true}).sort((a,b)=>a.name.localeCompare(b.name,'en'))){const file=path.join(dir,entry.name);if(entry.isSymbolicLink())throw Error('Reference tree symlink not supported');if(entry.isDirectory())visit(file);else if(entry.isFile())entries.push([path.relative(root,file).replace(/\\/g,'/'),sha(file)]);}}
 visit(root);return crypto.createHash('sha256').update(JSON.stringify(entries)).digest('hex');
}
function reuseFrame(seedRoot,plan,index,reference,test){
 if(!seedRoot)return null;
 const planFile=path.join(seedRoot,'plan.json');if(!fs.existsSync(planFile))return null;const previous=read(planFile);
 // Legacy plans without a full calculation fingerprint are deliberately recomputed.
 if(!previous.calculation_sha256||previous.calculation_sha256!==plan.calculation_sha256)return null;
 for(const key of ['reference_id','test_id','reference_sha256','test_sha256','metadata_sha256','executable_sha256','fftw_sha256','decoded_frames','ppd','zero_based_phase','stride','png_pixel_format'])if(previous[key]!==plan[key])return null;
 if(JSON.stringify(previous.indices)!==JSON.stringify(plan.indices)||JSON.stringify(previous.reference_timestamps)!==JSON.stringify(plan.reference_timestamps)||JSON.stringify(previous.test_timestamps)!==JSON.stringify(plan.test_timestamps))return null;
 const frame=path.join(seedRoot,'frame'+String(index).padStart(4,'0')),file=path.join(frame,'comparison.json');if(!fs.existsSync(file))return null;const r=read(file);
 if(!r.exact)return null;
 if(r.zero_based_frame!==index||r.timestamp!==plan.reference_timestamps[index]||r.official.Q_hex!==r.cpp.Q_hex||r.official.Q_JOD_hex!==r.cpp.Q_JOD_hex)throw Error('Claimed reusable frame is inconsistent');
 for(const hex of [r.official.Q_hex,r.official.Q_JOD_hex]){if(!/^[0-9a-f]{8}$/i.test(hex))throw Error('Reuse requires Single scores');const b=Buffer.alloc(4);b.writeUInt32LE(parseInt(hex,16));if(!Number.isFinite(b.readFloatLE()))throw Error('Reuse score nonfinite');}
 if(r.official.reference_version!=='3.0.7'||r.official.backend!=='CPU'||r.official.input_precision!=='single'||r.official.ppd!==plan.ppd)throw Error('Reuse reference settings inconsistent');
 const refHash=sha(reference),testHash=sha(test);
 if(refHash!==r.reference_png_sha256||testHash!==r.test_png_sha256)return null;
 if(sha(path.join(frame,'decoded/reference.png'))!==refHash||sha(path.join(frame,'decoded/test.png'))!==testHash||r.official.inputs[0].source_sha256!==refHash||r.official.inputs[1].source_sha256!==testHash)throw Error('Reuse source provenance corrupted');
 return {official:r.official,cpp:r.cpp,source_comparison:path.resolve(file),source_comparison_sha256:sha(file)};
}
module.exports={reuseFrame,fingerprintTree};
