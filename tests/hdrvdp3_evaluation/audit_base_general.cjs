// Read-only bit audit of saved GPU outputs; does not launch numerical work.
const fs=require('node:fs'),path=require('node:path');
if(process.argv.length!==3)throw Error('Usage: base-general-evidence-root');
const root=path.resolve(process.argv[2]);
function words(file){const t=fs.readFileSync(file,'utf8').trim().split(/\s+/).map(Number);if(t[0]!==1||t.length!==t[1]+2)throw Error('Word matrix shape');const b=Buffer.alloc(t[1]*4);t.slice(2).forEach((v,i)=>b.writeUInt32LE(v,4*i));return b;}
function equal(a,b,label){if(!a.equals(b))throw Error('Bit mismatch: '+label);}
const metadata=JSON.parse(fs.readFileSync(path.join(root,'fixtures_limits/metadata.json'),'utf8'));
if(metadata.length!==20||new Set(metadata.map(r=>r.case)).size!==20||new Set(metadata.map(r=>r.base_shape.join('x'))).size!==4)throw Error('Incomplete native scope');
let total=0,zeroLimits=0;
const limits=fs.readFileSync(path.join(root,'limit_acceptance/results.tsv'),'utf8').trim().split(/\r?\n/).slice(1).map(row=>row.split('\t'));
if(limits.length!==20)throw Error('Incomplete actual limit scope');
for(const r of metadata){
    const n=r.base_shape[0]*r.base_shape[1],dir=path.join(root,'fixtures_limits');
    for(const [stage,name] of [[1,'mean'],[2,'pack'],[3,'expected']]){
        const target=words(path.join(dir,r.case+'_'+name+'.txt'));
        equal(fs.readFileSync(path.join(root,'acceptance',r.case+'_stage'+stage+'.u32')),target,r.case+'/'+name);
        equal(words(path.join(root,'fixtures',r.case+'_'+name+'.txt')),target,r.case+'/fixture lineage');
        total+=target.length/4;
    }
    const spectrum=fs.readFileSync(path.join(root,'acceptance',r.case+'_stage3.u32'));
    if(spectrum.length!==(3*n+1)*4||spectrum.readFloatLE(3*n*4)!==r.index_zero_based)throw Error('Actual selected bin');
    equal(fs.readFileSync(path.join(root,'limit_acceptance',r.case+'_adaptation.f32')),words(path.join(dir,r.case+'_adapt.txt')),r.case+'/adaptation');
    const row=limits.find(v=>v[0]===r.case);if(!row||Number(row[1])!==r.index_zero_based||Number(row[5])!==0)throw Error('Limit row');
    const noise=row[4]==='inf'?Infinity:Number(row[4]);
    const actual=Buffer.alloc(16);actual.writeDoubleLE(Number(row[2]),0);actual.writeDoubleLE(noise,8);
    equal(actual,fs.readFileSync(path.join(dir,r.case+'_limits.bin')),r.case+'/frequency-noise');
    if(r.zero_frequency_noise_limit){if(noise!==Infinity)throw Error('Identity +Inf');zeroLimits++;}
}
if(total!==17000||zeroLimits!==4)throw Error('Wrong acceptance totals');
for(const name of ['acceptance','limit_acceptance','v2_pipeline_regression','spyr_acceptance','quality_acceptance/asymmetric','quality_acceptance/impulse','quality_acceptance/hdr_ramp','quality_acceptance/constant','quality_acceptance/identity','quality_acceptance/real']){
    if(!fs.existsSync(path.join(root,name,'PASS.txt')))throw Error('Missing PASS:'+name);
    if(/VUID-|Validation Error|\[ERROR\]/.test(fs.readFileSync(path.join(root,name+'.log'),'utf8')))throw Error('Vulkan errors:'+name);
}
let spyrTotal=0,qualityTotal=0;
for(const r of metadata)for(let stage=0;stage<11;stage++){
    const target=words(path.join(root,'spyr_goldens',r.case,'stage'+stage+'.txt'));
    equal(fs.readFileSync(path.join(root,'spyr_acceptance',r.case+'_stage'+stage+'.u32')),target,r.case+'/SPYR'+stage);
    spyrTotal+=target.length/4;
}
for(const name of ['asymmetric','impulse','hdr_ramp','constant','identity','real']){
    const rows=fs.readFileSync(path.join(root,'quality_acceptance',name,'results.tsv'),'utf8').trim().split(/\r?\n/).slice(1).map(row=>row.split('\t'));
    if(rows.length!==69)throw Error('Incomplete quality stages');
    for(const row of rows){if(+row[2]!==0||+row[3]!==0)throw Error('Quality mismatch');qualityTotal+=+row[1];}
}
if(spyrTotal!==1144720||qualityTotal!==1711662)throw Error('Incomplete full-chain totals');
console.log('Saved-output audit PASS:20 pairs/four shapes;17000 pipeline words +40 adaptation words +40 Float64 frequency/noise values exact;four +Inf identity limits.');
console.log('SPYR:220checks/1144720words;quality:414checks/1711662values;all exact. Cohorts overlap.');
