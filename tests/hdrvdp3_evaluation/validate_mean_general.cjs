// Strict isolated mean/DC acceptance; never promotes a runtime profile.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {spawnSync}=require('node:child_process');
if(process.argv.length!==6)throw Error('Usage: executable shader native-fixtures fresh-output');
const [exe,shader,native,out]=process.argv.slice(2).map(p=>path.resolve(p));
fs.mkdirSync(out);
const counts=[1,2,3,7,64,100,120,144,255,256,1024,4096],records=[];
for(const count of counts){
    const dest=path.join(out,'n'+count);
    const manifest=path.join(native,'n'+count,'manifest.txt');
    const args=[shader,manifest,dest,String(count)];
    const result=spawnSync(exe,args,{encoding:'utf8'});
    const log=(result.stdout||'')+(result.stderr||'');
    fs.writeFileSync(dest+'.log',log);
    if(result.error)throw result.error;
    if(result.status!==0||/VUID-|Validation Error|\[ERROR\]/.test(log)||!fs.existsSync(path.join(dest,'PASS.txt')))
        throw Error('GPU mean/DC failure: '+count);
    const rows=fs.readFileSync(path.join(dest,'results.tsv'),'utf8').trim().split(/\r?\n/).slice(1).map(row=>row.split('\t'));
    if(rows.length!==13||new Set(rows.map(row=>row[0])).size!==13)throw Error('Incomplete cohort');
    let synthetic=0,nativeValues=0;
    for(const [name,values,numeric,bits] of rows){
        if(+numeric!==0||+bits!==0)throw Error('Bit mismatch');
        if(name==='synthetic'){if(+values!==1024*(count+1))throw Error('Synthetic count');synthetic+=+values;}
        else {if(+values!==count+1)throw Error('Native count');nativeValues+=+values;}
    }
    if(nativeValues!==12*(count+1)||!synthetic)throw Error('Native coverage');
    records.push({count,syntheticProbes:1024,nativeProbes:12,syntheticValues:synthetic,nativeValues,bitErrors:0});
    fs.writeFileSync(path.join(out,'results.json'),JSON.stringify(records,null,2));
    console.log(`mean/DC count=${count}: ${synthetic+nativeValues} words bitwise PASS`);
}
const hashes={};
for(const file of [exe,shader,__filename,path.join(native,'metadata.json')])
    hashes[file]=crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
fs.writeFileSync(path.join(out,'hashes.json'),JSON.stringify(hashes,null,2));
const total=records.reduce((n,r)=>n+r.syntheticValues+r.nativeValues,0);
fs.writeFileSync(path.join(out,'PASS.txt'),`12 counts; 12288 bounded synthetic probes and144 independent Octave mean/DC probes; ${total} Float32 words bitwise; no reported Vulkan validation errors. Isolated component only.\n`);
