// Audit saved accepted outputs only. Failed candidates remain untouched.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
if(process.argv.length!==3)throw Error('Usage: square80-evidence-root');
const root=path.resolve(process.argv[2]),names=['asymmetric','impulse','hdr_ramp','constant','identity'];
function equal(a,b,label){if(!a.equals(b))throw Error('Bit mismatch:'+label);}
function log(file){const b=fs.readFileSync(file);return b[0]===255&&b[1]===254?b.toString('utf16le'):b.toString('utf8');}
function accepted(file){if(/VUID-|Validation Error|\[ERROR\]/.test(log(file)))throw Error('Accepted log errors:'+file);}
function rows(dir){return fs.readFileSync(path.join(dir,'results.tsv'),'utf8').trim().split(/\r?\n/).slice(1).map(r=>r.split('\t'));}
let apiValues=0,visualValues=0;
for(const name of names){
 const dir=path.join(root,'public_api_final',name),r=rows(dir);
 if(r.length!==69)throw Error('API coverage');
 for(const [stage,count,numeric,bits]of r){if(+numeric||+bits)throw Error('API mismatch');apiValues+=+count;equal(fs.readFileSync(path.join(dir,stage+'.bin')),fs.readFileSync(path.join(dir,stage+'_expected.bin')),name+'/'+stage);}
 accepted(path.join(root,'public_api_final',name+'.log'));
 const visual=rows(path.join(root,'spyr43_acceptance',name));if(visual.length!==43)throw Error('Visual coverage');
 for(const row of visual){if(+row[2]||+row[3])throw Error('Visual mismatch');visualValues+=+row[1];}
 accepted(path.join(root,'spyr43_acceptance',name+'.log'));
 const cli=path.join(root,'public_cli',name);
 for(const [file,target]of [['input_reference.f32','reference.txt'],['input_test.f32','test.txt']]){
  const t=fs.readFileSync(path.join(root,'quality_native','native_'+name,'h80_w80',target),'utf8').trim().split(/\s+/).map(Number);
  if(t[0]!==80||t[1]!==80||t.length!==6402)throw Error('Native input shape');
  const expected=Buffer.alloc(6400*4);t.slice(2).forEach((v,i)=>expected.writeFloatLE(v,4*i));equal(fs.readFileSync(path.join(cli,file)),expected,name+'/'+file);
 }
 for(const [file,stage]of [['reference_P.f32','reference_P'],['test_P.f32','test_P'],['log_adaptation.f32','log'],['scores.f32','scores']])equal(fs.readFileSync(path.join(cli,file)),fs.readFileSync(path.join(dir,stage+'.bin')),name+'/'+file);
 if(!fs.readFileSync(path.join(cli,'pathway_profile.txt'),'utf8').startsWith('v3;80x80;'))throw Error('Wrong provenance');
 accepted(path.join(root,'public_cli',name+'.log'));
}
if(apiValues!==990565||visualValues!==5164555)throw Error('Comparison totals');
const pins=JSON.parse(fs.readFileSync('tests/hdrvdp3_evaluation/REFERENCE_SQUARE80_PATHWAY_HASHES.json','utf8').replace(/^\uFEFF/,''));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const manifest=sha(path.join(root,'runtime/pathway.runtime'));
for(const dir of ['runtime','source_build/runtime','iso/runtime','runtime_documented']){
 for(const pin of pins)if(sha(path.join(root,dir,pin.file))!==pin.sha256.toLowerCase())throw Error('Payload:'+dir+'/'+pin.key);
 if(sha(path.join(root,dir,'pathway.runtime'))!==manifest)throw Error('Manifest:'+dir);
}
for(const file of ['contracts_v1/PASS.txt','contracts_v2/PASS.txt','contracts_v3/PASS.txt','production_regression_v3/PASS.txt','api_regression/PASS.txt','normalization_acceptance/PASS.txt','final_cli_smoke/PASS.txt','geometry_contract_PASS.txt','viewing_contract_PASS.txt'])if(!fs.existsSync(path.join(root,file)))throw Error('Missing:'+file);
accepted(path.join(root,'normalization_acceptance.log'));accepted(path.join(root,'final_cli_smoke.log'));
console.log('Accepted square80 audit PASS:345 API checks/990565 values;215 visual checks/5164555 words;five CLI pairs;18 payloads and manifest match across accepted/live/isolated/documented packages. Cohorts overlap.');
