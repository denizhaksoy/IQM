const fs=require('fs'),p=require('path'),cp=require('child_process'),crypto=require('crypto');const root=process.argv[2],source=p.join(root,'accepted_pack/runtime_repro/runtime'),out=p.join(root,'v10_rejections');fs.mkdirSync(out);const command=JSON.parse(fs.readFileSync(p.join(root,'public_v10/stress_oled/command.json'),'utf8')),records=[];
const cases=['missing-logarithm','bad-digest','bad-magic','v9-extra','duplicate','missing-file',...['projection','mutual','neighbor','pyramid','comparison','jnd','csf','quality'].map(s=>'missing-'+s)];
for(const name of cases){const dir=p.join(out,name);fs.cpSync(source,dir,{recursive:true,errorOnExist:true,force:false});let m=fs.readFileSync(p.join(dir,'double96.runtime'),'utf8');
 if(name.startsWith('missing-')&&name!=='missing-file')m=m.replace(new RegExp('^'+name.slice(8)+' .*\\r?\\n','m'),'');
 if(name==='v9-extra')m=m.replace('DOUBLE96 10','DOUBLE96 9');
 if(name==='duplicate')m+='\n'+m.match(/^logarithm .*$/m)[0];
 if(name==='missing-file')m=m.replace('shaders/logarithm.spv','shaders/absent.spv');
 if(name==='bad-digest'||name==='bad-magic'){const file=p.join(dir,'shaders/logarithm.spv'),b=fs.readFileSync(file);b.writeUInt32LE(0,0);fs.writeFileSync(file,b);if(name==='bad-magic'){const hash=crypto.createHash('sha256').update(b).digest('hex');m=m.replace(/^(logarithm\s+"[^"]+"\s+)\S+/m,'$1'+hash);}}
 fs.writeFileSync(p.join(dir,'double96.runtime'),m);const args=command.args.slice();args[args.indexOf('--hdrvdp3-runtime')+1]=dir;args[args.indexOf('--hdrvdp3-export')+1]=p.join(dir,'unexpected-output');const r=cp.spawnSync(command.exe,args,{encoding:'utf8',windowsHide:true});if(r.error)throw r.error;const log=r.stdout+r.stderr;fs.writeFileSync(p.join(out,name+'.log'),log);if(r.status===0||/VUID-|\[ERROR\]|Using "NVIDIA/.test(log))throw Error('Unexpected runtime result '+name);records.push({name,exit:r.status});
}
fs.writeFileSync(p.join(root,'v10_runtime_rejections.json'),JSON.stringify(records,null,2));console.log('PASS:14 v10 runtime rejections before GPU creation');
