const fs=require('fs'),path=require('path'),cp=require('child_process');
const [exe,inputs,runtime,data,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: executable PF-input-directory RGB-runtime raw-data fresh-output');fs.mkdirSync(out,{recursive:true});
const small=path.join(out,'small.pfm');fs.writeFileSync(small,Buffer.concat([Buffer.from('PF\n1 1\n-1\n'),Buffer.alloc(12)]));
const base=['--method','HDRVDP3','--input',path.join(inputs,'test.pfm'),'--ref',path.join(inputs,'reference.pfm'),'--hdrvdp3-runtime',runtime,'--hdrvdp3-encoding','rgb-native','--hdrvdp3-display','oled','--hdrvdp3-spectral-data',data];
function set(key,value,source=base){const a=[...source],i=a.indexOf(key);if(i<0)throw Error('Missing argument');a[i+1]=value;return a;}
const missing=[...base],index=missing.indexOf('--hdrvdp3-spectral-data');missing.splice(index,2);
const cases=[
 ['shape',set('--input',small,set('--ref',small)),/supports96x96 PF inputs only/],
 ['encoding',set('--hdrvdp3-encoding','srgb'),/Unsupported HDRVDP3 encoding/],
 ['display',set('--hdrvdp3-display','unknown'),/RGB requires --hdrvdp3-display/],
 ['ppd',[...base,'--hdrvdp3-ppd','60'],/RGB supports30ppd\/age24 only/],
 ['age60',[...base,'--hdrvdp3-age','60'],/RGB supports30ppd\/age24 only/],
 ['age60branch',[...base,'--hdrvdp3-age','60.001'],/RGB supports30ppd\/age24 only/],
 ['runtime',set('--hdrvdp3-runtime',path.join(runtime,'base')),/Unsupported RGB runtime profile/],
 ['spectral',missing,/RGB requires --hdrvdp3-spectral-data/],
 ['luminance_display',set('--hdrvdp3-encoding','luminance'),/Display option requires rgb-native encoding/]
];
for(const [tag,args,expected] of cases){const dest=path.join(out,tag),run=cp.spawnSync(path.resolve(exe),[...args,'--hdrvdp3-export',dest],{encoding:'utf8',windowsHide:true}),log=(run.stdout||'')+(run.stderr||'');fs.writeFileSync(path.join(out,tag+'.log'),log);if(run.error||run.status===0||fs.existsSync(dest)||!expected.test(log)||/Quality API returned/.test(log))throw Error('Incorrect profile rejection: '+tag);}
fs.writeFileSync(path.join(out,'PASS.txt'),'9 unsupported RGB geometry/encoding/display/viewing/runtime/spectral combinations rejected with expected errors and no export directory.\n');console.log('RGB profile rejections PASS:9');
