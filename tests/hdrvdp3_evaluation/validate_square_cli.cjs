// Node is only the fixture writer, process launcher and strict result auditor.
// All image-dependent numerical processing remains in the public CLI.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {spawnSync} = require('node:child_process');
function fail(message) { throw new Error(message); }
const [exeArg, runtimeArg, sourceArg, outputArg, sideArg] = process.argv.slice(2);
const side=sideArg===undefined?96:Number(sideArg);
if (![6,7].includes(process.argv.length)||![80,96,128].includes(side)) fail('Usage: node validate_pathway_hdr_cli.cjs executable runtime square-quality-native-root fresh-output');
const exe = path.resolve(exeArg), runtime = path.resolve(runtimeArg);
const source = path.resolve(sourceArg), output = path.resolve(outputArg);
fs.mkdirSync(output); // Must be fresh; never reuse numerical output.
const provenance=JSON.parse(fs.readFileSync(path.join(source,'asset_sources.json'),'utf8'));
const apiRoot=provenance.public_api?path.resolve(provenance.public_api):path.join(source,'acceptance');
const sha = data => crypto.createHash('sha256').update(data).digest('hex');
function equal(a, b, label) { if (!a.equals(b)) fail(`Bit mismatch: ${label}`); }
function matrix(file) {
    const tokens = fs.readFileSync(file, 'utf8').trim().split(/\s+/).map(Number);
    const [h,w] = tokens;
    if (!Number.isInteger(h) || !Number.isInteger(w) || h <= 0 || w <= 0 || tokens.length !== h*w+2) fail(`Invalid matrix: ${file}`);
    const bytes = Buffer.alloc(h*w*4);
    tokens.slice(2).forEach((x,i) => {
        if (!Number.isFinite(x) || !Number.isFinite(Math.fround(x))) fail(`Nonfinite: ${file}`);
        bytes.writeFloatLE(x,4*i);
    });
    return {h,w,bytes};
}
function pfm(file, m) {
    const header = Buffer.from(`Pf\n${m.w} ${m.h}\n-1.0\n`, 'ascii');
    const pixels = Buffer.alloc(m.bytes.length);
    for (let y=0; y<m.h; ++y) m.bytes.copy(pixels,(m.h-1-y)*m.w*4,y*m.w*4,(y+1)*m.w*4);
    fs.writeFileSync(file, Buffer.concat([header,pixels]), {flag:'wx'});
    // Independent byte-level readback, including the PFM row inversion.
    const readback = fs.readFileSync(file);
    equal(readback.subarray(0,header.length),header,'PFM header');
    if (readback.length !== header.length+m.bytes.length) fail('PFM length');
    for (let y=0; y<m.h; ++y) equal(readback.subarray(header.length+(m.h-1-y)*m.w*4,header.length+(m.h-y)*m.w*4),m.bytes.subarray(y*m.w*4,(y+1)*m.w*4),'PFM row');
}
const records=[];
for (const name of (side===128?['asymmetric','impulse','hdr_ramp','constant','identity','competing_axes']:side===80?['asymmetric','impulse','hdr_ramp','constant','identity']:['asymmetric','impulse','hdr_ramp','constant','identity','real'])) {
        const tag=name, native=path.join(source,'native_'+name,`h${side}_w${side}`);
        const api=path.join(apiRoot,name), dest=path.join(output,tag);
        const ref=matrix(path.join(native,'reference.txt')), test=matrix(path.join(native,'test.txt'));
        if (ref.h!==test.h || ref.w!==test.w) fail('Input shape mismatch');
        const refFile=path.join(output,`${tag}_ref.pfm`), testFile=path.join(output,`${tag}_test.pfm`);
        pfm(refFile,ref); pfm(testFile,test);
        const args=['--method','HDRVDP3','--enable-validation','--input',testFile,'--ref',refFile,'--hdrvdp3-runtime',runtime,'--hdrvdp3-export',dest];
        fs.writeFileSync(path.join(output,`${tag}_command.json`),JSON.stringify({exe,args,cwd:output},null,2));
        const run=spawnSync(exe,args,{cwd:output,encoding:'utf8',windowsHide:true,maxBuffer:16*1024*1024});
        const log=(run.stdout||'')+(run.stderr||'');
        fs.writeFileSync(path.join(output,`${tag}.log`),log);
        if (run.error || run.status!==0 || /VUID-|Validation Error|\[ERROR\]/.test(log)) fail(`CLI failure ${tag}: ${run.error||run.status}`);
        equal(fs.readFileSync(path.join(dest,'input_reference.f32')),ref.bytes,'CLI reference input');
        equal(fs.readFileSync(path.join(dest,'input_test.f32')),test.bytes,'CLI test input');
        for (const [cli,nativeFile,apiFile] of [
            ['reference_P.f32','P_reference.txt','reference_P.bin'],
            ['test_P.f32','P_test.txt','test_P.bin'],
            ['log_adaptation.f32','log_La.txt','log.bin'],
            ['scores.f32','scores.txt','scores.bin']]) {
            const actual=fs.readFileSync(path.join(dest,cli));
            equal(actual,matrix(path.join(native,nativeFile)).bytes,`${tag}/${cli} native`);
            equal(actual,fs.readFileSync(path.join(api,apiFile)),`${tag}/${cli} API`);
        }
        const quality=fs.readFileSync(path.join(dest,'quality.txt'),'utf8').trim().split(/\s+/).map(Number);
        const expected=Buffer.concat(['band_errors.txt','Q_error.txt','scores.txt'].map(f=>matrix(path.join(native,f)).bytes));
        if (quality.length!==8 || quality.some(x=>!Number.isFinite(x))) fail('Invalid quality text');
        const actual=Buffer.alloc(32);quality.forEach((x,i)=>actual.writeFloatLE(x,4*i));
        equal(actual,expected,`${tag}/quality native`);
        equal(actual,Buffer.concat(['band_errors.bin','Q_error.bin','scores.bin'].map(f=>fs.readFileSync(path.join(api,f)))),`${tag}/quality API`);
        const scores=fs.readFileSync(path.join(dest,'scores.f32'));
        records.push({case:tag,Q:scores.readFloatLE(0),Q_JOD:scores.readFloatLE(4),exports:7,bit_errors:0});
        fs.writeFileSync(path.join(output,'results.json'),JSON.stringify(records,null,2));
        console.log(`${tag}: seven CLI exports bitwise PASS`);
}
const hashes={};
for (const f of [exe,__filename,path.join(runtime,'pathway.runtime'),path.join(source,'asset_sources.json')]) hashes[f]=sha(fs.readFileSync(f));
fs.writeFileSync(path.join(output,'hashes.json'),JSON.stringify(hashes,null,2));
fs.writeFileSync(path.join(output,'PASS.txt'),`${side===128?'Six square128':side===80?'Five square80':'Six square96'} pairs; seven exports each match original inputs/native/API bits; quality text errors and scores match; PFM readback exact; no Vulkan validation errors.\n`);
