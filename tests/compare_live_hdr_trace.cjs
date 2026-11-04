const fs=require('node:fs'),path=require('node:path');
const [preprocessing,official,cpp,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: preprocessing official-capture CPP-trace fresh-report.json');
const entries=fs.readFileSync(path.join(official,'classes.jsonl'),'utf8').trim().split('\n').map(JSON.parse),results=[];
function compare(name,aFile,bFile,h,w,channels,transpose){
 const a=fs.readFileSync(aFile),b=fs.readFileSync(bFile);if(a.length!==h*w*channels*4||b.length!==a.length)throw Error('Shape differs: '+name);
 const aw=new Uint32Array(a.buffer,a.byteOffset,a.length/4),bw=new Uint32Array(b.buffer,b.byteOffset,b.length/4),af=new Float32Array(a.buffer,a.byteOffset,a.length/4),bf=new Float32Array(b.buffer,b.byteOffset,b.length/4);
 let count=0,maximum=0,first=null;for(let c=0;c<channels;c++)for(let y=0;y<h;y++)for(let x=0;x<w;x++){const i=c*h*w+y*w+x,j=transpose?c*h*w+x*h+y:i;if(aw[j]!==bw[i]){count++;maximum=Math.max(maximum,Math.abs(af[j]-bf[i]));first??={channel:c,y,x,official:af[j],cpp:bf[i],official_hex:aw[j].toString(16),cpp_hex:bw[i].toString(16)};}}
 results.push({name,values:aw.length,bit_mismatches:count,max_abs:maximum,first});console.log(name+': '+count+'/'+aw.length);
}
for(const side of ['reference','test'])compare(side+'_linear',path.join(preprocessing,side+'.f32'),path.join(cpp,side+'_linear.f32'),2160,3840,3,false);
for(const e of entries){const file=path.join(cpp,e.tag+'.f32');if(e.class==='single'&&fs.existsSync(file)&&fs.existsSync(path.join(official,e.tag+'.f32'))){const [h,w,c=1]=e.shape;compare(e.tag,path.join(official,e.tag+'.f32'),file,h,w,c,true);}}
fs.writeFileSync(out,JSON.stringify({scope:'stage localization, explicit row/column-major conversion',results},null,2));
