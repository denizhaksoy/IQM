// Audit saved accepted outputs only. Failed candidates remain untouched.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
if(process.argv.length!==3)throw Error('Usage: square128-evidence-root');
const root=path.resolve(process.argv[2]),names=['asymmetric','impulse','hdr_ramp','constant','identity','competing_axes'];
function equal(a,b,label){if(!a.equals(b))throw Error('Bit mismatch:'+label);}
function log(file){const b=fs.readFileSync(file);return b[0]===255&&b[1]===254?b.toString('utf16le'):b.toString('utf8');}
function accepted(file){if(/VUID-|Validation Error|\[ERROR\]/.test(log(file)))throw Error('Accepted log errors:'+file);}
function rows(dir){return fs.readFileSync(path.join(dir,'results.tsv'),'utf8').trim().split(/\r?\n/).slice(1).map(r=>r.split('\t'));}
let apiValues=0,visualValues=0;
for(const name of names){
 const dir=path.join(root,'public_api_v4',name),r=rows(dir);
 if(r.length!==69)throw Error('API coverage');
 for(const [stage,count,numeric,bits]of r){if(+numeric||+bits)throw Error('API mismatch');apiValues+=+count;equal(fs.readFileSync(path.join(dir,stage+'.bin')),fs.readFileSync(path.join(dir,stage+'_expected.bin')),name+'/'+stage);}
 accepted(path.join(root,'public_api_v4',name+'.log'));
 const visual=rows(path.join(root,'visual43_all',name));if(visual.length!==43)throw Error('Visual coverage');
 for(const row of visual){if(+row[2]||+row[3])throw Error('Visual mismatch');visualValues+=+row[1];}
 accepted(path.join(root,'visual43_all',name+'.log'));
 const cli=path.join(root,'public_cli_v4',name);
 for(const [file,target]of [['input_reference.f32','reference.txt'],['input_test.f32','test.txt']]){
  const t=fs.readFileSync(path.join(root,'quality_native','native_'+name,'h128_w128',target),'utf8').trim().split(/\s+/).map(Number);
  if(t[0]!==128||t[1]!==128||t.length!==16386)throw Error('Native input shape');
  const expected=Buffer.alloc(16384*4);t.slice(2).forEach((v,i)=>expected.writeFloatLE(v,4*i));equal(fs.readFileSync(path.join(cli,file)),expected,name+'/'+file);
 }
 for(const [file,stage]of [['reference_P.f32','reference_P'],['test_P.f32','test_P'],['log_adaptation.f32','log'],['scores.f32','scores']])equal(fs.readFileSync(path.join(cli,file)),fs.readFileSync(path.join(dir,stage+'.bin')),name+'/'+file);
 const profile=fs.readFileSync(path.join(cli,'pathway_profile.txt'),'utf8');
 if(!profile.startsWith('v4;128x128;')||!profile.includes(';map14;'))throw Error('Wrong provenance');
 accepted(path.join(root,'public_cli_v4',name+'.log'));
}
if(apiValues!==3042894||visualValues!==15865410)throw Error('Comparison totals');
function nativeWords(file){const t=fs.readFileSync(file,'utf8').trim().split(/\s+/).map(Number);if(t[0]!==1||t.length!==t[1]+2)throw Error('Native word shape');const b=Buffer.alloc(t[1]*4);t.slice(2).forEach((v,i)=>b.writeUInt32LE(v,4*i));return b;}
let baseValues=0;
for(const name of names)for(const [stage,field]of [[1,'mean'],[2,'pack'],[3,'expected']]){
 const tag='h128_w128_'+name,expected=nativeWords(path.join(root,'fixtures_six',tag+'_'+field+'.txt'));
 equal(fs.readFileSync(path.join(root,'acceptance_six',tag+'_stage'+stage+'.u32')),expected,tag+'/'+field);baseValues+=expected.length/4;
}
if(baseValues!==10770)throw Error('Base coverage');
const stress=fs.readFileSync(path.join(root,'acceptance_six/h128_w128_competing_axes_stage3.u32'));
if(stress.readFloatLE(768*4)!==32)throw Error('Near-tie selected index');
const magnitudes=Array.from({length:256},(_,i)=>stress.readUInt32LE((512+i)*4));
const ranked=[...new Set(magnitudes)].sort((a,b)=>b-a);
if(ranked[0]-ranked[1]!==1||magnitudes.filter(v=>v===ranked[0]).length!==2)throw Error('One-ULP near-tie not preserved');
const original='validation/quality128_stress_639282594471943896/native_competing_axes';
for(const name of ['reference','test']){
 const t=fs.readFileSync(path.join(root,'native_competing_axes/h128_w128',name+'.txt'),'utf8').trim().split(/\s+/).map(Number);
 const pfm=fs.readFileSync(path.join(original,name+'.pfm')),header=Buffer.from('Pf\n128 128\n-1.0\n');
 equal(pfm.subarray(0,header.length),header,'Historical PFM header');
 const expected=Buffer.alloc(16384*4);for(let y=0;y<128;y++)for(let x=0;x<128;x++)expected.writeFloatLE(t[2+y*128+x],4*((127-y)*128+x));
 equal(pfm.subarray(header.length),expected,'Historical pair input bits/'+name);
}
const pins=JSON.parse(fs.readFileSync('tests/hdrvdp3_evaluation/REFERENCE_SQUARE128_PATHWAY_HASHES.json','utf8').replace(/^\uFEFF/,''));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const manifest=sha(path.join(root,'runtime_v4/pathway.runtime'));
for(const dir of ['runtime_v4','src/runtime','iso/runtime','runtime_documented_v4']){
 for(const pin of pins)if(sha(path.join(root,dir,pin.file))!==pin.sha256.toLowerCase())throw Error('Payload:'+dir+'/'+pin.key);
 if(sha(path.join(root,dir,'pathway.runtime'))!==manifest)throw Error('Manifest:'+dir);
}
for(const file of ['contracts_v1/PASS.txt','contracts_v2/PASS.txt','contracts_v3/PASS.txt','contracts_v4/PASS.txt','cli_old_regression/PASS.txt','public_regression_v4/PASS.txt','acceptance_six/PASS.txt','spyr_acceptance_six/PASS.txt','limit_acceptance_six/PASS.txt','old_base_regression/PASS.txt','old_spyr_regression/PASS.txt','profile_viewing_contract_PASS.txt','final_cli_help_PASS.txt'])if(!fs.existsSync(path.join(root,file)))throw Error('Missing:'+file);
console.log('Accepted square128 audit PASS:414 API checks/3042894 values;258 visual checks/15865410 words;six CLI pairs;18 payloads and manifest match across accepted/live/isolated/documented packages. Cohorts overlap.');
