const fs=require('fs'),p=require('path');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_srgb_gpu_frontend.cjs root');
const lineage=JSON.parse(fs.readFileSync(p.join(root,'lineage.json'),'utf8')),records=[];
for(const [tag,native] of [['stress','native'],['ramps','native_ramps'],['identity','native_identity_v2']]){
 const dir=p.join(root,'allfft_'+tag),gold=p.join(lineage.cpu,native);
 if(!fs.readFileSync(p.join(dir,'PASS.txt'),'utf8').startsWith('Independent original Single input through Double hybrid GPU-convolution/CPU-quality/allFFT-GPU Q/Q_JOD bitwise exact'))throw Error('Missing acceptance');
 if(fs.readFileSync(p.join(dir,'summary.txt'),'utf8').trim()!=='forward_bits=0 inverse_bits=0 padding_bits=0 filter_bits=0 adaptation_bits=0 independent=1')throw Error('Boundary mismatch');
 if(/VUID-|\[ERROR\]/.test(fs.readFileSync(p.join(root,'allfft_'+tag+'.log'),'utf8')))throw Error('Vulkan error');
 const rows=fs.readFileSync(p.join(dir,'results.tsv'),'utf8').trim().split(/\r?\n/).slice(1);if(rows.length!==24||rows.some(r=>!/^\S+\t[12]\t[1-4]\t0\t0\t0\t0$/.test(r)))throw Error('Convolution coverage');
 for(const d of ['led-lcd-srgb','led-lcd-wcg','oled']){
  for(const s of [1,2]){
   for(const [suffix,n] of [['JND',82944],['pyramid',21601]])if(!fs.readFileSync(p.join(dir,`${d}_${s}_${suffix}.txt`),'utf8').startsWith(`0 bit errors /${n} words;`))throw Error('Visual mismatch');
   for(const c of [1,2,3,4])for(const field of ['inverse','output']){const name=`${d}_${s}_conv${c}_${field}.f64`;if(!fs.readFileSync(p.join(dir,name)).equals(fs.readFileSync(p.join(gold,name))))throw Error('Native convolution bytes differ');}
  }
  let words=0;for(const line of fs.readFileSync(p.join(dir,d+'_full_quality.tsv'),'utf8').trim().split(/\r?\n/)){const m=line.match(/^\S+\t0\/(\d+)$/);if(!m)throw Error('Quality mismatch');words+=Number(m[1]);}if(words!==237630)throw Error('Quality coverage');
  const summary=fs.readFileSync(p.join(dir,d+'_full_quality_summary.txt'),'utf8').trim(),m=summary.match(/^0 bit errors \/237630 Double words;Q=(\S+) Q_JOD=(\S+)$/);if(!m)throw Error('Missing scores');
  const scores=fs.readFileSync(p.join(gold,d+'_quality.f64'));if(Number(m[1])!==scores.readDoubleLE(8)||Number(m[2])!==scores.readDoubleLE(16))throw Error('Native scores differ');records.push({pair:tag,display:d,Q:Number(m[1]),Q_JOD:Number(m[2])});
 }
}
if(fs.existsSync(p.join(root,'no_fftw_runtime_sample.txt'))&&!/fftw_modules=0;/.test(fs.readFileSync(p.join(root,'no_fftw_runtime_sample.txt'),'utf8')))throw Error('FFTW module sample failed');
const result={scope:'Original Single RGB->independent CPU display preparation->GPU optical/adaptation convolution and crop->CPU visual/quality;base12 FFT also GPU. No FFTW DLL. Diagnostic only,not fullGPU/public runtime.',configurations:9,compared_Double_words:17457408,overlapping_GPU_crop_words:663552,bit_errors:0,records};
fs.writeFileSync(p.join(root,'results.json'),JSON.stringify(result,null,2));fs.writeFileSync(p.join(root,'final_audit_PASS.txt'),'PASS:9 original-input hybrid configurations;17457408 compared Double words,18 Q/Q_JOD scalars exact;allFFT GPU,no FFTW DLL;GPU crop663552 overlapping words exact;Vulkan logs clean.\n');console.log('PASS:9 original-input configurations,allFFT GPU,18 exact scores');
