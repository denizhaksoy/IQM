// Additional content pairs through the real PF CLI; no numerical processing here.
const fs=require('fs'),path=require('path'),cp=require('child_process');
const [exe,nativeRoot,runtime,data,out,mode]=process.argv.slice(2);
if(mode&&mode!=='--real-only')throw Error('Unknown content mode');
if(!out||fs.existsSync(out))throw Error('Usage: executable native-root RGB-runtime raw-spectral-data fresh-output');fs.mkdirSync(out,{recursive:true});
function rows(b,planes){const r=Buffer.alloc(b.length),n=96*96;for(let c=0;c<planes;c++)for(let y=0;y<96;y++)for(let x=0;x<96;x++)b.copy(r,4*(c*n+y*96+x),4*(c*n+x*96+y),4*(c*n+x*96+y+1));return r;}
function pf(p,r){const b=Buffer.alloc(r.length);let k=0;for(let y=95;y>=0;y--)for(let x=0;x<96;x++)for(let c=0;c<3;c++){r.copy(b,k,4*(c*96*96+y*96+x),4*(c*96*96+y*96+x+1));k+=4;}fs.writeFileSync(p,Buffer.concat([Buffer.from('PF\n96 96\n-1\n'),b]));}
let total=0,scalars=0;const records=[];
const patterns=mode==='--real-only'?[['real','oled']]:[['identity','led-lcd-srgb'],['constant','led-lcd-srgb'],['primaries','oled'],['impulse','oled']];
for(const [pattern,display] of patterns){
 const native=path.join(nativeRoot,'native_'+pattern),dir=path.join(out,pattern);fs.mkdirSync(dir);const inputs={};
 for(const [side,label] of [[1,'reference'],[2,'test']]){const input=rows(fs.readFileSync(path.join(native,`${display}_${side}_input.f32`)),3);inputs[label]=path.join(dir,label+'.pfm');pf(inputs[label],input);}
 const dest=path.join(dir,'export');const args=['--method','HDRVDP3','--enable-validation','--input',inputs.test,'--ref',inputs.reference,'--hdrvdp3-runtime',runtime,'--hdrvdp3-encoding','rgb-native','--hdrvdp3-display',display,'--hdrvdp3-spectral-data',data,'--hdrvdp3-export',dest];
 fs.writeFileSync(path.join(dir,'command.json'),JSON.stringify({exe:path.resolve(exe),args,cwd:process.cwd()},null,2));
 const run=cp.spawnSync(path.resolve(exe),args,{encoding:'utf8',windowsHide:true,maxBuffer:16*1024*1024}),log=(run.stdout||'')+(run.stderr||'');fs.writeFileSync(path.join(dir,'cli.log'),log);
 if(run.error||run.status!==0||/VUID-|Validation Error|\[ERROR\]/.test(log))throw Error(pattern+' CLI failed: '+(run.error||run.status));
 for(const [side,label] of [[1,'reference'],[2,'test']])for(const [suffix,golden,planes] of [['rgb','input',3],['optical','optical',3],['LMSR','response',4],['adaptation','adaptation',1],['P','P',1]]){
  const a=fs.readFileSync(path.join(dest,label+'_'+suffix+'.f32')),e=rows(fs.readFileSync(path.join(native,`${display}_${side}_${golden}.f32`)),planes);if(!a.equals(e))throw Error(pattern+' '+label+'_'+suffix+' bits differ');total+=a.length/4;
 }
 const q=fs.readFileSync(path.join(dest,'quality.txt'),'utf8').trim().split(/\s+/).map(Number),e=Buffer.concat([fs.readFileSync(path.join(native,display+'_errors.f32')),fs.readFileSync(path.join(native,display+'_quality.f32'))]);
 if(q.length!==8)throw Error('Quality output shape');for(let i=0;i<8;i++){const b=Buffer.alloc(4);b.writeFloatLE(q[i]);if(b.readUInt32LE()!==e.readUInt32LE(4*i))throw Error(pattern+' quality bits differ');scalars++;}
 records.push({pattern,display,Q:q[6],Q_JOD:q[7],bit_errors:0});fs.writeFileSync(path.join(out,'results.json'),JSON.stringify(records,null,2));console.log(pattern+' RGB CLI native bits PASS');
}
if(total!==patterns.length*221184||scalars!==patterns.length*8)throw Error('Coverage mismatch');fs.writeFileSync(path.join(out,'PASS.txt'),`${patterns.length} additional RGB96 content pairs;${total} exported Float32 words +${scalars} quality scalars bitwise. Overlaps API cohort.\n`);
