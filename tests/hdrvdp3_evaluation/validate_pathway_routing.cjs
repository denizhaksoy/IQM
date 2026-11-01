// Regression for modern-manifest routing and exported profile provenance.
const fs=require('node:fs'),path=require('node:path'),{spawnSync}=require('node:child_process');
const crypto=require('node:crypto');
if(process.argv.length!==6)throw Error('Usage: executable square-public-evidence v1-runtime fresh-output');
// Four arguments after the script.
const [exe,baseline,v1,out]=process.argv.slice(2).map(p=>path.resolve(p));
fs.mkdirSync(out);
const runtime=path.join(baseline,'runtime'),records=[];
function run(tag,args){
    const r=spawnSync(exe,args,{encoding:'utf8'}),log=(r.stdout||'')+(r.stderr||'');
    fs.writeFileSync(path.join(out,tag+'.log'),log);
    records.push({tag,args,status:r.status});
    if(r.error)throw r.error;
    if(/VUID-|Validation Error|\[ERROR\]/.test(log))throw Error('Vulkan validation: '+tag);
    return {status:r.status,log};
}
const files=['input_reference.f32','input_test.f32','reference_P.f32','test_P.f32','log_adaptation.f32','scores.f32','quality.txt'];
for(const [tag,input,expected,assets,label] of [
    ['square96',path.join(baseline,'cli_acceptance/real'),path.join(baseline,'final_cli_smoke'),runtime,'v2;96x96;'],
    ['rectangle',path.join(baseline,'v1_cli/inputs/asymmetric_h80_w96'),path.join(baseline,'v1_cli/asymmetric_h80_w96'),v1,'v1;96x80;']]){
    const dest=path.join(out,tag);
    const r=run(tag,['--method','HDRVDP3','--enable-validation','--input',input+'_test.pfm','--ref',input+'_ref.pfm','--hdrvdp3-runtime',assets,'--hdrvdp3-export',dest]);
    if(r.status!==0)throw Error('CLI failed: '+tag);
    for(const f of files)if(!fs.readFileSync(path.join(dest,f)).equals(fs.readFileSync(path.join(expected,f))))throw Error('Bit mismatch: '+tag+'/'+f);
    if(!fs.readFileSync(path.join(dest,'pathway_profile.txt'),'utf8').startsWith(label))throw Error('Profile label: '+tag);
}
const pfm=path.join(out,'unsupported128.pfm'),pixels=Buffer.alloc(128*128*4);
for(let i=0;i<128*128;i++)pixels.writeFloatLE(100,i*4);
fs.writeFileSync(pfm,Buffer.concat([Buffer.from('Pf\n128 128\n-1.0\n'),pixels]));
const rejected=path.join(out,'must_not_exist');
const r=run('unsupported128',['--method','HDRVDP3','--enable-validation','--input',pfm,'--ref',pfm,'--hdrvdp3-runtime',runtime,'--hdrvdp3-export',rejected]);
if(r.status===0||!(r.log.includes('Runtime profile does not support requested geometry')||r.log.includes('Pathway runtime requires matching'))||fs.existsSync(rejected))throw Error('Unsupported shape must fail in modern route before export');
fs.writeFileSync(path.join(out,'runs.json'),JSON.stringify(records,null,2));
fs.writeFileSync(path.join(out,'binary_sha256.txt'),crypto.createHash('sha256').update(fs.readFileSync(exe)).digest('hex')+'\n');
fs.writeFileSync(path.join(out,'PASS.txt'),'Square96 and v1 rectangle retain seven exports byte-for-byte; v2/v1 provenance labels correct; modern128 rejected before export; no reported Vulkan errors.\n');
console.log('Modern routing, v1/v2 exports and provenance PASS');
