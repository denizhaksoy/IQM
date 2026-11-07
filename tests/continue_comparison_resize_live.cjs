const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve('validation/comparison_resize_live_20261106_run01'),out=path.resolve('validation/comparison_resize_live_continuation_20261107_run01');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 if(fs.existsSync(out))throw Error('Fresh output required');
 const p=read(path.join(root,'provenance.json')),old=fs.readFileSync(path.join(root,'results/results.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse),checkpoint=read(path.join(root,'results/checkpoint.json'));
 if(checkpoint.complete||old.length!==checkpoint.completed_frames||old.some((r,i)=>!r.exact||r.case!==p.inputs[i].id||r.dispatches!==8||r.double_values!==2754000))throw Error('Invalid preserved prefix');
 fs.mkdirSync(out);fs.copyFileSync(__filename,path.join(out,'launcher_snapshot.cjs'));
 const runtime=path.join(root,'runtime');
 function verifyRuntime(){if(sha(path.join(runtime,'probe.exe'))!==p.exe_sha256||sha(path.join(runtime,'resize.spv'))!==p.shader_sha256)throw Error('Runtime changed');for(const m of p.model)if(sha(path.join(runtime,'model',m.file))!==m.sha256)throw Error('Model changed');}
 verifyRuntime();
 for(const s of p.inputs)if(sha(s.reference)!==s.reference_sha256||sha(s.test)!==s.test_sha256)throw Error('Input changed');
 const remaining=p.inputs.slice(old.length),list=path.join(out,'remaining.tsv');fs.writeFileSync(list,remaining.map(s=>[s.id,s.reference,s.test].join('\t')).join('\n')+'\n');
 fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify({original_provenance_sha256:sha(path.join(root,'provenance.json')),preserved_results_sha256:sha(path.join(root,'results/results.jsonl')),preserved_frames:old.length,remaining_frames:remaining.length,scope:'stage only; not full quality'},null,2));
 await new Promise((resolve,reject)=>{const child=cp.spawn(path.join(runtime,'probe.exe'),[list,path.join(runtime,'model'),path.join(runtime,'resize.spv'),path.join(out,'results')],{windowsHide:true});child.stdout.on('data',b=>fs.appendFileSync(path.join(out,'probe.log'),b));child.stderr.on('data',b=>fs.appendFileSync(path.join(out,'probe.log'),b));child.on('error',reject);child.on('close',code=>code===0?resolve():reject(Error('Probe exit '+code)));});
 const rows=fs.readFileSync(path.join(out,'results/results.jsonl'),'utf8').trim().split(/\r?\n/).map(JSON.parse),end=read(path.join(out,'results/checkpoint.json'));
 if(!end.complete||!end.passed||rows.length!==remaining.length||rows.some((r,i)=>!r.exact||r.case!==remaining[i].id||r.dispatches!==8||r.double_values!==2754000))throw Error('Invalid remaining results');
 if(/\[ERROR\]|Validation Error|VUID-/i.test(fs.readFileSync(path.join(out,'probe.log'),'utf8')+fs.readFileSync(path.join(root,'probe.log'),'utf8')))throw Error('Vulkan error');
 verifyRuntime();for(const s of p.inputs)if(sha(s.reference)!==s.reference_sha256||sha(s.test)!==s.test_sha256)throw Error('Input changed');
 fs.writeFileSync(path.join(out,'completion_audit.json'),JSON.stringify({complete:true,passed:true,unique_frames:old.length+rows.length,double_values:2754000*(old.length+rows.length),preserved_frames:old.length,new_frames:rows.length,scope:'Double comparison resize only; not full quality or paper reproduction'},null,2));
}
main().catch(e=>{if(fs.existsSync(out))fs.writeFileSync(path.join(out,'failure.txt'),e.stack);console.error(e.stack);process.exitCode=1;});
