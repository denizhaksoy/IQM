// Independent PNG-to-score execution; no intermediate reference tensors supplied.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const [exe,reference,test,data,dll,projection,mask,stockPath,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: exe ref.png test.png model fftw-dll projection.spv mask.spv official-score.json fresh-out');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'),stock=read(stockPath);
if(stock.reference_version!=='3.0.7'||stock.input_precision!=='single'||stock.backend!=='CPU'||stock.ppd!==62.729130063099447||stock.fftw_threads!==3||stock.fftw_planner!=='estimate'||stock.inputs[0].source_sha256!==sha(reference)||stock.inputs[1].source_sha256!==sha(test))throw Error('Official input/profile mismatch');
fs.mkdirSync(out,{recursive:true});
const files=[exe,reference,test,dll,projection,mask,stockPath,...fs.readdirSync(data).map(f=>path.join(data,f))];
const provenance=files.map(file=>({file:path.resolve(file),sha256:sha(file)}));
fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify({scope:'independent experimental matrix+mask hybrid, not full GPU',files:provenance},null,2));
// Freeze the executable/shaders before launching, so subsequent builds cannot
// alter this particular run or the provenance of its numerical result.
const frozenExe=path.join(out,'used.exe'),frozenProjection=path.join(out,'projection.spv'),frozenMask=path.join(out,'mask.spv');
fs.copyFileSync(exe,frozenExe);fs.copyFileSync(projection,frozenProjection);fs.copyFileSync(mask,frozenMask);
const args=['--input',path.resolve(test),'--ref',path.resolve(reference),'--encoding','pq-bt2020-rgb16','--ppd',String(stock.ppd),'--model-data',path.resolve(data),'--fftw',path.resolve(dll),'--projection-shader',path.resolve(frozenProjection),'--comparison-shader',path.resolve(frozenMask),'--output',path.resolve(out,'results')];
const run=cp.spawnSync(path.resolve(frozenExe),args,{encoding:'utf8',windowsHide:true,maxBuffer:16*1024*1024});
fs.writeFileSync(path.join(out,'stdout.log'),run.stdout||'');fs.writeFileSync(path.join(out,'stderr.log'),run.stderr||'');
if(run.error)throw run.error;if(run.status!==0)throw Error('Experimental application failed; inspect logs');
for(const e of provenance)if(sha(e.file)!==e.sha256)throw Error('Input/runtime changed while evaluating: '+e.file);
const actual=read(path.join(out,'results/score.json')),stages=read(path.join(out,'results/gpu_stages.json'));
const validationErrors=/\[ERROR\]|Validation Error|VUID-/i.test((run.stdout||'')+(run.stderr||''));
const exact=actual.Q_hex===stock.Q_hex&&actual.Q_JOD_hex===stock.Q_JOD_hex;
const backendVerified=actual.backend==='experimental-vulkan-matrices-mask-cpu-fftw'&&actual.gpu_matrix_dispatches===4&&stages.matrix_dispatches===4&&stages.comparison_dispatches===1&&stages.full_gpu===false;
fs.writeFileSync(path.join(out,'comparison.json'),JSON.stringify({scope:'independent PNG-to-score, Vulkan matrices+comparison mask; remaining stages CPU',exact,backendVerified,validationErrors,Q_hex:actual.Q_hex,reference_Q_hex:stock.Q_hex,Q_JOD_hex:actual.Q_JOD_hex,reference_Q_JOD_hex:stock.Q_JOD_hex,args},null,2));
if(!exact||!backendVerified||validationErrors)throw Error('Experimental matrix+mask profile not accepted');
console.log('EXACT Q/Q_JOD; 4 matrix + 1 comparison GPU dispatches; no reported validation errors');
