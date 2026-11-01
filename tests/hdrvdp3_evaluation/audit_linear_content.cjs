// Audit existing runs without rerunning Vulkan or overwriting their exports.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=process.argv[2];if(!root)throw Error('Usage: node audit_linear_content.cjs root');
const records=[];let values=0,scalars=0;
for(const encoding of ['rgb-bt.2020','xyz'])for(const pattern of ['identity','constant','primaries','impulse','real']){
 const native=path.join(root,`native_${encoding}_${pattern}`);
 if(JSON.parse(fs.readFileSync(path.join(native,'metadata.json'))).encoding!==encoding)throw Error('Wrong native encoding');
 if(!fs.readFileSync(path.join(native,'PASS.txt'),'utf8').includes('stock/traced score bits exact'))throw Error('Missing native acceptance');
 const dir=path.join(root,`cli_${encoding}_${pattern}`),pass=fs.readFileSync(path.join(dir,'PASS.txt'),'utf8');
 if(!pass.includes(`608256 Float32 exports +24 quality scalars bitwise;three ${encoding}`))throw Error('Missing acceptance '+dir);
 for(const display of ['led-lcd-srgb','led-lcd-wcg','oled']){
  const run=path.join(dir,display),log=fs.readFileSync(path.join(run,'cli.log'),'utf8');
  if(/Validation Error|VUID-|\[ERROR\]/.test(log))throw Error('Validation error '+run);
  const command=JSON.parse(fs.readFileSync(path.join(run,'command.json')));
  if(!command.args.includes('--enable-validation'))throw Error('Validation disabled');
  const quality=fs.readFileSync(path.join(run,'export','quality.txt'),'utf8').trim().split(/\s+/).map(Number);
  if(quality.length!==8||quality.some(v=>!Number.isFinite(v)))throw Error('Invalid quality');
  records.push({encoding,pattern,display,Q:quality[6],Q_JOD:quality[7],exe:command.exe});
 }
 values+=608256;scalars+=24;
}
const probe=JSON.parse(fs.readFileSync(path.join(root,'srgb_probe','metadata.json')));
if(probe.records.length!==3||probe.records.some(r=>r.Float32_roundtrip_bit_changes!==55296||r.visual_input_type!=='double'))throw Error('Precision evidence missing');
const executables=[...new Set(records.map(r=>r.exe))];if(executables.length!==1)throw Error('Mixed executables');
const hash=crypto.createHash('sha256').update(fs.readFileSync(executables[0])).digest('hex');
const summary={scope:'Five content pairs interpreted under two linear encodings and three spectra;96x96/30ppd/age24/defaultsp0. Not whole-metric parity.',Float32_export_words:values,quality_scalars:scalars,configurations:records.length,executable_sha256:hash,records,srgb_precision_probe:probe};
fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(summary,null,2));
fs.writeFileSync(path.join(root,'final_audit_PASS.txt'),`${values} Float32 export words + ${scalars} quality scalars bitwise; ${records.length} configurations. Vulkan logs clean. Native-only sRGB precision probe separate.\n`);
console.log('PASS',values,scalars,records.length);
