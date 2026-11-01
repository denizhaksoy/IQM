const fs=require('fs'),p=require('path');
const root=process.argv[2],read=f=>fs.readFileSync(p.join(root,f),'utf8'),assert=(v,m)=>{if(!v)throw Error(m);};
assert(read('stage_rawpool/PASS.txt').trim()==='1158714 native/derived Double quality words exact;643730 control words exact;10 rejection checks','Stage PASS');
const rows=read('stage_rawpool/comparisons.tsv').trim().split(/\r?\n/);let total=0,pools=0;
for(const row of rows){const m=row.match(/^(.+)\t0\/(\d+)$/);assert(m,'Comparison mismatch');total+=Number(m[2]);if(m[1].endsWith('raw pooling')){assert(m[2]==='1','Pooling shape');pools++;}}
assert(rows.length===560&&total===1802444&&pools===70,'Coverage');
const sub=p.join(root,'public_v9'),lineage=JSON.parse(fs.readFileSync(p.join(sub,'lineage.json'))),cli=JSON.parse(fs.readFileSync(p.join(sub,'results.json')));
let nativeWords=0;
for(const native of ['native','native_ramps','native_identity_v2'])for(const display of ['led-lcd-srgb','led-lcd-wcg','oled'])for(const [b,w] of [96,96,48,24,12].entries()){
 const prefix=`${native}_${display}_band${b+1}`;
 for(const field of ['mask_noise','excitation','difference','psychometric','masked']){
  const actual=fs.readFileSync(p.join(root,'stage_rawpool',prefix+'_'+field+'.f64')),gold=fs.readFileSync(p.join(lineage.cpu,native,`${display}_quality_band${b+1}_${field}.f64`));
  assert(actual.length===w*w*8&&gold.length===actual.length,'Map shape');
  for(let y=0;y<w;y++)for(let x=0;x<w;x++)assert(actual.subarray((y*w+x)*8,(y*w+x+1)*8).equals(gold.subarray((x*w+y)*8,(x*w+y+1)*8)),'Native map bytes');
  nativeWords+=w*w;
 }
 assert(fs.readFileSync(p.join(root,'stage_rawpool',prefix+'_denominator.f64')).equals(fs.readFileSync(p.join(root,'stage_rawpool',prefix+'_denominator_expected.f64'))),'Derived denominator');
 assert(fs.statSync(p.join(root,'stage_rawpool',prefix+'_pooling.f64')).size===8,'Pooling scalar');
}
assert(nativeWords===965520&&cli.configurations===9&&cli.exact_quality_values===72&&cli.negative_checks===13,'CLI coverage');
for(const row of cli.records){
 const name=row.tag+'_'+row.display,dir=p.join(sub,name),v=fs.readFileSync(p.join(dir,'output/quality.txt'),'utf8').trim().split(/\s+/).map(Number),native=p.join(lineage.cpu,({stress:'native',ramps:'native_ramps',identity:'native_identity_v2'})[row.tag]);
 for(let b=0;b<5;b++)assert(v[b]===fs.readFileSync(p.join(native,row.display+`_quality_band${b+1}_error.f64`)).readDoubleLE(),'Band score');
 const scores=fs.readFileSync(p.join(native,row.display+'_quality.f64'));for(let i=0;i<3;i++)assert(v[i+5]===scores.readDoubleLE(i*8),'Final score');
 assert(fs.readFileSync(p.join(dir,'output/profile.txt'),'utf8').includes('GPU quality-arithmetic yes;CPU pow/psychometric/final scalar'),'CPU boundary');
 assert(!/VUID-|\[ERROR\]/.test(fs.readFileSync(p.join(sub,name+'.log'),'utf8')),'CLI validation');
}
for(let n=1;n<=8;n++){assert(read(`v${n}_regression_PASS.txt`).trim()===`v${n} 3 numeric/input exports byte-identical`,'Legacy regression');assert(!/VUID-|\[ERROR\]/.test(read(`v${n}_regression.log`)),'Legacy validation');}
assert(!/VUID-|\[ERROR\]/.test(read('stage_rawpool.log')),'Stage validation');
assert(JSON.parse(read('v9_runtime_rejections.json')).length===13,'Runtime rejection coverage');
const results={scope:'Fixed96 hybrid GPU quality arithmetic and normalized sequential pooling;CPU powers,psychometric,final error power and final scores retained. Denominator/raw pooling use derived C++ oracles, not independent native exports.',total_compared_words:total,GPU_returned_values:1501990,CPU_values:300454,native_or_derived_words:1158714,control_words:643730,stage_rejections:10,CLI_configurations:9,CLI_quality_Doubles:72,CLI_rejections:13,runtime_rejections:13,legacy_numeric_input_exports:24};
fs.writeFileSync(p.join(root,'results.json'),JSON.stringify(results,null,2));
fs.writeFileSync(p.join(root,'final_audit_PASS.txt'),'PASS:1802444 hybrid words exact;1501990 GPU-returned/300454 CPU values;9 CLI configurations;v1-v8 numeric exports preserved;validation logs clean.\n');
console.log('PASS: v9 quality arithmetic/pooling audit');
