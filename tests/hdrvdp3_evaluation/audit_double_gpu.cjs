const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_double_gpu.cjs root');
const source=fs.readFileSync(path.join(root,'native_source_path.txt'),'utf8').trim(),records=[];
const displays=['led-lcd-srgb','led-lcd-wcg','oled'];
for(const [tag,native,axis] of [['stress','native','axis_native'],['ramps','native_ramps','axis_native_ramps'],['identity','native_identity_v2','axis_native_identity']]){
 for(const [kind,count] of [['base',864],['axis',893952]]){
  const name=`final_${kind}_${tag}`,dir=path.join(root,name),log=fs.readFileSync(path.join(root,name+'.log'),'utf8');
  if(/VUID-|\[ERROR\]/.test(log))throw Error('Vulkan validation error');
  if(fs.readFileSync(path.join(dir,'results.txt'),'utf8').trim()!==`0 bit errors;0 numeric errors /${count} Double words`)throw Error('Numeric mismatch');
  if(!fs.readFileSync(path.join(dir,'PASS.txt'),'utf8').startsWith(`${count} Double GPU`))throw Error('Missing pass');
  const expected=kind==='axis'?fs.readFileSync(path.join(root,axis,'expected.f64')):Buffer.concat(displays.map(d=>fs.readFileSync(path.join(source,native,`${d}_quality_band5_base_fft.f64`))));
  const actual=fs.readFileSync(path.join(dir,'actual.f64'));if(actual.length!==count*8||!actual.equals(expected))throw Error('Binary mismatch');
  records.push({case:tag,stage:kind,Double_words:count});
 }
}
for(const name of ['device_selection','legacy_cli'])if(/VUID-|\[ERROR\]/.test(fs.readFileSync(path.join(root,name+'.log'),'utf8')))throw Error('Regression validation error');
if(!fs.existsSync(path.join(root,'device_selection/PASS.txt'))||!fs.readFileSync(path.join(root,'legacy_cli_PASS.txt'),'utf8').startsWith('16 existing BT2020'))throw Error('Missing regressions');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const results={scope:'Isolated Double GPU FFT stages;not full GPU metric. Two distorted pairs plus identity control;three display spectra. NVIDIA RTX A500,driver595.95.',base12_words:2592,real192_first_axis_words:2681856,total_Double_words:2684448,legacy_export_files_identical:16,unsupported_device_rejections:2,shader12_sha256:hash(path.join(root,'repro/fft12_double.spv')),shader192_sha256:hash(path.join(root,'axis192_repro/real192_double.spv')),twiddle_sha256:hash(path.join(root,'axis192_repro/twiddles.f64')),records};
fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(results,null,2));fs.writeFileSync(path.join(root,'final_audit_PASS.txt'),'2684448 Double GPU FFT words bitwise;12x12 base and real192 first-axis only. Validation logs clean;default-device/Float64 negative checks pass;16 legacy CLI files byte-identical. Not full2D192 or full GPU metric.\n');console.log('PASS:2684448 Double GPU words,16 legacy files');
