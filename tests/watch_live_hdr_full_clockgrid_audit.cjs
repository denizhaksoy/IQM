// Automatically run the independent, read-only audit once full evaluation passes.
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve('validation/live_hdr_full_scores_20261107_run03'),audit='tests/audit_live_hdr_full_clockgrid.cjs',out=path.join(root,'independent_audit.json'),status=path.join(root,'audit_watcher.json');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 if(fs.existsSync(status))throw Error('Watcher already registered');
 const expected=hash(audit);fs.copyFileSync(audit,path.join(root,'audit_snapshot.cjs'));
 const save=stage=>fs.writeFileSync(status,JSON.stringify({stage,time:new Date().toISOString(),audit_sha256:expected},null,2));save('waiting_for_full_evaluation');
 while(true){
  if(fs.existsSync(path.join(root,'failure.txt'))||fs.existsSync(path.join(root,'status_failed.json')))throw Error('Full evaluation failed; independent acceptance not attempted');
  const p=JSON.parse(fs.readFileSync(path.join(root,'checkpoint.json'),'utf8'));
  if(p.complete){if(!p.passed)throw Error('Full evaluation not accepted');break;}
  await new Promise(r=>setTimeout(r,30000));
 }
 if(hash(audit)!==expected)throw Error('Audit source changed');save('auditing');
 const run=cp.spawnSync(process.execPath,[audit,'validation/live_hdr_full_archive_20261106_run01/full_archive_cohort.json',root,out],{windowsHide:true,encoding:'utf8',maxBuffer:16*1024*1024});
 fs.writeFileSync(path.join(root,'independent_audit.log'),(run.stdout||'')+(run.stderr||''));if(run.error)throw run.error;if(run.status!==0)throw Error('Independent audit failed; inspect log');
 const result=JSON.parse(fs.readFileSync(out,'utf8'));if(!result.complete||!result.passed||result.videos!==279||result.frames!==4716)throw Error('Audit coverage differs');save('complete');
}
main().catch(e=>{fs.writeFileSync(path.join(root,'audit_watcher_failure.txt'),e.stack);console.error(e.stack);process.exitCode=1;});
