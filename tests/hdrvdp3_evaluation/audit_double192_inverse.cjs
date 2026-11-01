const fs=require('fs'),p=require('path');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_double192_inverse.cjs root');
const source=fs.readFileSync(p.join(root,'native_source_path.txt'),'utf8').trim();
for(const [native,original] of [['native','native'],['native_ramps','native_ramps'],['native_identity','native_identity_v2']]){
 const expected=fs.readFileSync(p.join(root,native,'expected_real.f64'));const parts=[];
 for(const d of ['led-lcd-srgb','led-lcd-wcg','oled'])for(const s of [1,2])for(const c of [1,2,3,4])parts.push(fs.readFileSync(p.join(source,original,`${d}_${s}_conv${c}_inverse.f64`)));
 if(!expected.equals(Buffer.concat(parts)))throw Error('Fresh inverse differs from original native convolution');
 for(const [stage,file,count] of [['axis','expected_axis.f64',1769472],['inverse','expected_real.f64',884736]]){
  const name=`accepted_${stage}_${native}`,dir=p.join(root,name);
  if(!fs.readFileSync(p.join(dir,'actual.f64')).equals(fs.readFileSync(p.join(root,native,file))))throw Error('Bit mismatch '+name);
  if(fs.readFileSync(p.join(dir,'results.txt'),'utf8').trim()!==`0 bit errors;0 numeric errors /${count} Double words`)throw Error('Count mismatch');
  if(!fs.existsSync(p.join(dir,'PASS.txt')))throw Error('Missing PASS');
  if(/VUID-|\[ERROR\]/.test(fs.readFileSync(p.join(root,name+'.log'),'utf8')))throw Error('Vulkan validation error');
 }
}
if(fs.readFileSync(p.join(root,'regression_forward/results.txt'),'utf8').trim()!=='0 bit errors;0 numeric errors /1769472 Double words')throw Error('Forward regression failed');
if(/VUID-|\[ERROR\]/.test(fs.readFileSync(p.join(root,'regression_forward.log'),'utf8')))throw Error('Regression validation error');
fs.writeFileSync(p.join(root,'results.json'),JSON.stringify({scope:'Normalized real(ifft2) of native Double filtered spectrum;both inverse axes GPU;not full GPU filtering/metric',matrices:72,real_inverse_Double_words:2654208,overlapping_first_axis_component_words:5308416,bit_errors:0,forward_regression_words:1769472,device:'NVIDIA RTX A500 Laptop GPU',normalization:'FMA remainder corrected division;tested profile only,not proof of all-input correctly rounded division',profile:'96sq source,192sq padding,30ppd,age24,defaultsp0,3spectra,2distorted pairs plus identity control;DC unchanged'},null,2));
fs.writeFileSync(p.join(root,'final_audit_PASS.txt'),'PASS:72 normalized real inverse matrices,2654208 Double words bitwise exact;first-axis scope overlaps;fresh native references equal prior convolution captures;forward regression and Vulkan logs pass.\n');console.log('PASS:2654208 real inverse Double words');
