// Fresh synthetic PF fixtures, CPU decoding contracts, actual application CLI.
const fs=require('fs'),path=require('path'),cp=require('child_process');
const [exe,loader,native,runtime,data,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: CLI loader native RGB-runtime spectral-data fresh-output');
fs.mkdirSync(out,{recursive:true});
function run(binary,args,tag,success=true){const r=cp.spawnSync(path.resolve(binary),args,{encoding:'utf8',windowsHide:true});fs.writeFileSync(path.join(out,tag+'.log'),(r.stdout||'')+(r.stderr||''));if(r.error||success&&r.status!==0||!success&&r.status===0)throw Error(tag+': '+(r.error||r.status));return r.stdout;}
function transpose(b,w,h,channels){const a=Buffer.alloc(b.length);for(let c=0;c<channels;c++)for(let y=0;y<h;y++)for(let x=0;x<w;x++)b.copy(a,4*(c*w*h+y*w+x),4*(c*w*h+x*h+y),4*(c*w*h+x*h+y+1));return a;}
function pf(file,row,w,h,scale=-1,big=false){const raster=Buffer.alloc(row.length);let offset=0;for(let y=h-1;y>=0;y--)for(let x=0;x<w;x++)for(let c=0;c<3;c++){const v=row.readFloatLE(4*(c*w*h+y*w+x));if(big)raster.writeFloatBE(v,offset);else raster.writeFloatLE(v,offset);offset+=4;}fs.writeFileSync(file,Buffer.concat([Buffer.from(`PF\n${w} ${h}\n${scale}\n`),raster]));}
const files={};for(const [side,label] of [[1,'reference'],[2,'test']]){const row=transpose(fs.readFileSync(path.join(native,`led-lcd-srgb_${side}_input.f32`)),96,96,3);files[label]=path.join(out,label+'.pfm');pf(files[label],row,96,96);fs.writeFileSync(path.join(out,label+'.f32'),row);run(loader,[files[label],path.join(out,label+'.f32'),'96','96'],'load_'+label);}
const small=Buffer.alloc(18*4);for(let i=0;i<18;i++)small.writeFloatLE(i===0?0:Math.fround(i*.13),4*i);
for(const [tag,scale,big] of [['little',-1,false],['big',1,true],['scaled',-1.25,false]]){const input=path.join(out,tag+'.pfm'),expected=Buffer.alloc(small.length);pf(input,small,3,2,scale,big);for(let i=0;i<18;i++)expected.writeFloatLE(Math.fround(small.readFloatLE(4*i)*Math.abs(scale)),4*i);const gold=path.join(out,tag+'.f32');fs.writeFileSync(gold,expected);run(loader,[input,gold,'3','2'],'load_'+tag);}
const valid=fs.readFileSync(files.reference),header=Buffer.from('PF\n96 96\n-1\n');
const malformed={gray:Buffer.concat([Buffer.from('Pf\n96 96\n-1\n'),valid.subarray(header.length)]),truncated:valid.subarray(0,valid.length-1),extra:Buffer.concat([valid,Buffer.from([0])]),zero_scale:Buffer.from('PF\n1 1\n0\n'),nan_scale:Buffer.from('PF\n1 1\nnan\n'),overflow:Buffer.from('PF\n4294967295 4294967295\n-1\n'),bad_dimensions:Buffer.from('PF\n-1 1\n-1\n')};
for(const [tag,value] of [['negative',-1],['nan',NaN],['infinite',Infinity]]){const b=Buffer.from(valid);b.writeFloatLE(value,header.length);malformed[tag]=b;}
for(const [tag,bytes] of Object.entries(malformed)){const file=path.join(out,'bad_'+tag+'.pfm');fs.writeFileSync(file,bytes);run(loader,['--reject',file],'reject_'+tag);}
const common=['--method','HDRVDP3','--input',files.test,'--ref',files.reference,'--hdrvdp3-runtime',runtime,'--hdrvdp3-encoding','rgb-native','--hdrvdp3-spectral-data',data];
run(exe,[...common],'missing_display',false);
run(exe,[...common,'--hdrvdp3-display','oled','--hdrvdp3-ppd','60'],'wrong_ppd',false);
run(exe,[...common,'--hdrvdp3-display','oled','--hdrvdp3-age','60'],'wrong_age',false);
run(exe,[...common,'--hdrvdp3-display','unknown'],'wrong_display',false);
run(exe,[...common,'--hdrvdp3-display','oled','--hdrvdp3-encoding','srgb'],'duplicate_encoding',false);
let values=0,qualityValues=0;
for(const display of ['led-lcd-srgb','led-lcd-wcg','oled']){
 const exportDir=path.join(out,display);run(exe,[...common,'--hdrvdp3-display',display,'--hdrvdp3-export',exportDir,'--enable-validation'],'cli_'+display);
 for(const [side,label] of [[1,'reference'],[2,'test']])for(const [suffix,nativeSuffix,planes] of [['rgb','input',3],['optical','optical',3],['LMSR','response',4],['adaptation','adaptation',1],['P','P',1]]){
  const a=fs.readFileSync(path.join(exportDir,`${label}_${suffix}.f32`)),e=transpose(fs.readFileSync(path.join(native,`${display}_${side}_${nativeSuffix}.f32`)),96,96,planes);
  if(!a.equals(e))throw Error(display+': '+label+'_'+suffix+' bits differ');values+=a.length/4;
 }
 const q=fs.readFileSync(path.join(exportDir,'quality.txt'),'utf8').trim().split(/\s+/).map(Number),expected=Buffer.concat([fs.readFileSync(path.join(native,display+'_errors.f32')),fs.readFileSync(path.join(native,display+'_quality.f32'))]);
 if(q.length!==8)throw Error('Quality coverage');for(let i=0;i<q.length;i++){const a=Buffer.alloc(4);a.writeFloatLE(q[i]);if(a.readUInt32LE()!==expected.readUInt32LE(4*i))throw Error(display+' quality bits differ');qualityValues++;}
 const profile=fs.readFileSync(path.join(exportDir,'profile.txt'),'utf8');if(!profile.includes('encoding rgb-native')||!profile.includes('display '+display)||!profile.includes('planar-row-major-little-endian-f32'))throw Error('Profile mismatch');
 console.log(display+' CLI native bits PASS');
}
if(values!==663552||qualityValues!==24)throw Error('Coverage mismatch');
run(exe,[...common,'--hdrvdp3-display','oled','--hdrvdp3-export',path.join(out,'oled')],'existing_export',false);
fs.writeFileSync(path.join(out,'PASS.txt'),`3 RGB PF CLI pairs;${values} Float32 export words +${qualityValues} quality scalars bitwise;5 valid PF decoding runs/10 malformed PF rejects/6 CLI argument-export rejects.\n`);
