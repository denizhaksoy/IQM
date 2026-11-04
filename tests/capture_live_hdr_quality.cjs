// Extend a fresh instrumented copy only; require its score to match a supplied clean run.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const [official,preprocessing,stock,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: clean-root preprocessing known-clean-score fresh-root');
const base=cp.spawnSync(process.execPath,['tests/capture_live_hdr_single.cjs',official,preprocessing,stock,out],{encoding:'utf8',windowsHide:true});if(base.status!==0)throw Error(base.stderr);
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'),main=path.join(out,'instrumented/hdrvdp3.m'),before=sha(main),anchor='Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();',source=fs.readFileSync(main,'utf8');
if(source.split(anchor).length!==2)throw Error('Nonunique quality pooling anchor');
const capture="\niqm_capture_single(['quality_band' num2str(b) '_native'],D_p);\niqm_capture_single(['quality_error' num2str(b)],minkowski_sum(D_p(:),0.8));";
fs.writeFileSync(main,source.replace(anchor,anchor+capture));const after=sha(main),runner=path.join(out,'run_capture.m');fs.writeFileSync(runner,fs.readFileSync(runner,'utf8').replace(before,after));
const provenance=JSON.parse(fs.readFileSync(path.join(out,'provenance.json'),'utf8'));provenance.base_instrumented_sha256=before;provenance.instrumented_sha256=after;provenance.extra_capture='D_p and per-band Minkowski error after original accumulation';fs.writeFileSync(path.join(out,'provenance.json'),JSON.stringify(provenance,null,2));
console.log(path.resolve(out));
