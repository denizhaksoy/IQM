const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_srgb_full.cjs root');
const displays=['led-lcd-srgb','led-lcd-wcg','oled'],records=[];
for(const [tag,stage] of [['stress','stage_native'],['ramps','stage_native_ramps'],['identity','stage_identity_final']]){
 const dir=path.join(root,'accepted_'+tag);
 if(!fs.readFileSync(path.join(dir,'PASS.txt'),'utf8').startsWith('Independent original Single input through Double Q/Q_JOD bitwise exact'))throw Error('Missing full pass');
 if(!fs.readFileSync(path.join(root,stage,'PASS.txt'),'utf8').startsWith('712020 Double words exact;12 invalidchecks'))throw Error('Missing stage pass');
 if(fs.readFileSync(path.join(dir,'summary.txt'),'utf8').trim()!=='forward_bits=0 inverse_bits=0 padding_bits=0 filter_bits=0 adaptation_bits=0 independent=1')throw Error('Frontend mismatch');
 for(const display of displays){
  const summary=fs.readFileSync(path.join(dir,display+'_full_quality_summary.txt'),'utf8').trim();const m=summary.match(/^0 bit errors \/237630 Double words;Q=(\S+) Q_JOD=(\S+)$/);if(!m)throw Error('Invalid summary');
  let words=0;for(const line of fs.readFileSync(path.join(dir,display+'_full_quality.tsv'),'utf8').trim().split(/\r?\n/)){const v=line.match(/^\S+\t0\/(\d+)$/);if(!v)throw Error('Quality mismatch');words+=Number(v[1]);}if(words!==237630)throw Error('Quality coverage');
  for(const side of [1,2])for(const [suffix,n] of [['JND',82944],['pyramid',21601]])if(!fs.readFileSync(path.join(dir,`${display}_${side}_${suffix}.txt`),'utf8').startsWith(`0 bit errors /${n} words;`))throw Error('Stage mismatch');
  const Q=Number(m[1]),Q_JOD=Number(m[2]);if(!Number.isFinite(Q)||!Number.isFinite(Q_JOD)||(tag==='identity'&&(Q!==10||Q_JOD!==10)))throw Error('Invalid scores');records.push({pair:tag,display,Q,Q_JOD});
 }
}
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const result={scope:'Independent original Single encoded images to Double CPU Q/Q_JOD;96x96/30ppd/age24/defaultsp0. Two distorted pairs plus identity control;three spectra. Local Octave FFTW diagnostic,not production GPU/whole-metric coverage.',configurations:9,independent_Double_words:17457408,overlapping_stage_Double_words:2136060,repeated_invalid_checks:36,executable_sha256:hash(path.join(root,'build/Release/srgb-fft-double-inspect.exe')),records};
fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(result,null,2));fs.writeFileSync(path.join(root,'final_audit_PASS.txt'),'9 CPU configurations;17457408 Double words bitwise,original encoded Single input to Q/Q_JOD. No production GPU/FFT/whole-metric acceptance.\n');console.log('PASS:9 configurations,17457408 Double words,18 Q/Q_JOD scalars');
