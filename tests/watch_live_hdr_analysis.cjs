// Attach final independent statistics verification without restarting a live pipeline.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
async function main(){
 const [configFile,analysisRoot]=process.argv.slice(2);if(!analysisRoot)throw Error('Usage: scheduler-config analysis-root');
 const config=read(configFile),base=path.dirname(path.resolve(configFile)),cohortFile=path.resolve(base,config.cohort),cohort=read(cohortFile),schedulerLock=path.resolve(base,config.output,'scheduler.lock');
 const source=path.join(analysisRoot,'analysis.json'),target=path.join(analysisRoot,'verification_octave.json'),lock=analysisRoot+'.verification.lock',status=analysisRoot+'.verification_status.json';
 if(fs.existsSync(target)||fs.existsSync(status))throw Error('Preserve prior verification/status; choose a fresh verification run');
 const fd=fs.openSync(lock,'wx');fs.writeFileSync(fd,JSON.stringify({pid:process.pid}));fs.closeSync(fd);
 const save=value=>fs.writeFileSync(status,JSON.stringify({time:new Date().toISOString(),...value},null,2));
 try{
  save({stage:'waiting',source:path.resolve(source)});let ticks=0,missing=0,report;
  while(true){
   if(fs.existsSync(source)){try{report=read(source);break;}catch(e){if(!(e instanceof SyntaxError))throw e;}}
   if(fs.existsSync(schedulerLock)){const owner=read(schedulerLock);if(!Number.isSafeInteger(owner.pid)||owner.pid<=0)throw Error('Invalid scheduler PID');process.kill(owner.pid,0);missing=0;}else if(++missing>6)throw Error('Scheduler disappeared without a readable final analysis');
   if(ticks++%12===0)console.log('Waiting for complete-cohort analysis; independent Octave verification queued');
   await new Promise(resolve=>setTimeout(resolve,5000));
  }
  const expected=new Set(cohort.cases.map(c=>c.video));
  if(report.cohort_sha256!==sha(cohortFile)||report.videos!==expected.size||report.rows.length!==expected.size||new Set(report.rows.map(r=>r.video)).size!==expected.size||report.rows.some(r=>!expected.has(r.video)))throw Error('Final analysis cohort differs');
  save({stage:'verifying',analysis_sha256:sha(source)});
  await new Promise((resolve,reject)=>{const c=cp.spawn('C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',['--quiet','tests/verify_live_hdr_cohort_analysis.m',source,target],{stdio:'inherit',windowsHide:true});c.on('error',reject);c.on('close',code=>code===0?resolve():reject(Error('Octave verification exited '+code)));});
  const verified=read(target);if(!verified.passed||verified.analysis_sha256!==sha(source))throw Error('Verification provenance differs');
  save({stage:'passed',videos:report.videos,analysis_sha256:verified.analysis_sha256,verification:path.resolve(target)});
 }catch(e){save({stage:'failed',error:String(e.stack)});throw e;}finally{fs.unlinkSync(lock);}
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
