const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [root]=process.argv.slice(2);if(!root)throw Error('Usage: completed-GPU-root');
const run=JSON.parse(fs.readFileSync(path.join(root,'run.json'),'utf8'));const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
if(run.exit!==0||run.reportedValidationErrors||sha(run.executable)!==run.executable_sha256||sha(run.args[3])!==run.shader_sha256)throw Error('GPU run/provenance mismatch');
const out=path.join(root,'snapshot');if(fs.existsSync(out))throw Error('Preserve snapshot');fs.mkdirSync(out);
const files={};for(const side of ['reference','test']){
 files[side+'_optical']=path.join(run.args[0],side,'optical.f32');files[side+'_expected_lmsr']=path.join(run.args[1],side+'_lmsr.f32');
 if(run.args[6]){files[side+'_linear']=path.join(run.args[6],side+'.f32');files[side+'_expected_native']=path.join(run.args[1],side+'_native.f32');}
}
for(const name of ['ciexyz31.csv','emission_spectra_oled.csv','log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt'])files[name]=path.join(run.args[2],name);
const tensors=Object.fromEntries(Object.entries(files).map(([k,p])=>[k,{path:path.resolve(p),sha256:sha(p)}]));
for(const [source,target] of [[run.executable,'used.exe'],[run.args[3],'used.spv'],['shaders/hdrvdp3/hdrvdp3_rgb_projection_2d.comp','shader.comp'],['src/hdrvdp3/projection_single_gpu.cpp','projection_single_gpu.cpp'],['include/IQM/hdrvdp3/projection_single_gpu.h','projection_single_gpu.h'],['tests/verify_live_hdr_gpu_projection.cpp','test.cpp']])fs.copyFileSync(source,path.join(out,target));
fs.writeFileSync(path.join(out,'inputs.json'),JSON.stringify({scope:'GPU matrix stages only; hashes recorded after completed numerical run',input_layout:'planar-row-major-f32',reference_layout:'planar-column-major-f32',expected_transposed_for_comparison:true,width:3840,height:2160,coefficient_precision:'CPU Double calibration cast to Single',padding:'none',DC_processing:'none',tensors},null,2));
console.log('GPU numerical run, binary, shader, models and input tensors bound to hashes');
