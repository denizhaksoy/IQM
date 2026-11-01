const fs=require('fs'),p=require('path'),crypto=require('crypto');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_double192_chain.cjs root');
const lineage=JSON.parse(fs.readFileSync(p.join(root,'lineage.json'),'utf8'));
for(const native of ['native','native_ramps','native_identity_v2']){
 const name='accepted_crop_'+native,dir=p.join(root,name);const real=[],crop=[];
 for(const d of ['led-lcd-srgb','led-lcd-wcg','oled'])for(const s of [1,2])for(const c of [1,2,3,4]){const prefix=p.join(lineage.native,native,`${d}_${s}_conv${c}`);real.push(fs.readFileSync(prefix+'_inverse.f64'));crop.push(fs.readFileSync(prefix+'_output.f64'));}
 if(!fs.readFileSync(p.join(dir,'actual.f64')).equals(Buffer.concat(real)))throw Error('Inverse mismatch '+native);
 if(!fs.readFileSync(p.join(dir,'cropped.f64')).equals(Buffer.concat(crop)))throw Error('Crop mismatch '+native);
 if(fs.readFileSync(p.join(dir,'results.txt'),'utf8').trim()!=='0 bit errors /884736 real Double words;source filter errors=0;crop errors=0 /221184')throw Error('Count mismatch');
 if(!fs.existsSync(p.join(dir,'PASS.txt')))throw Error('Missing pass');
 if(/VUID-|\[ERROR\]/.test(fs.readFileSync(p.join(root,name+'.log'),'utf8')))throw Error('Validation error');
}
const result={scope:'Native padded Double input->GPU forward both axes->source-generated CPU spectral coefficients/GPU multiply->GPU inverse both axes->GPU96sq crop. Not original-image/full GPU visual or quality.',matrices:72,inverse_real_Double_words:2654208,overlapping_cropped_Double_words:663552,source_generated_filter_bit_errors:0,bit_errors:0,device:'NVIDIA RTX A500 Laptop GPU',profile:'Single96sq upstream,Double192sq padding,30ppd,age24,defaultsp0,3spectra,2distorted pairs plus identity;DC unchanged;inverse36864;RTE64/signedzero64;no denorm64 guarantee'};
fs.writeFileSync(p.join(root,'results.json'),JSON.stringify(result,null,2));fs.writeFileSync(p.join(root,'final_audit_PASS.txt'),'PASS:72 forward/filter/inverse GPU matrices,2654208 real Double words plus overlapping663552 GPU-cropped words exact;generated filters exact;no Vulkan validation errors.\n');console.log('PASS:72 GPU convolution matrices and96sq crop bitwise exact');
