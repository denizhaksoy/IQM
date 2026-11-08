const fs=require('fs'),path=require('path');const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh output required');const base='validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/dft/scalar/codelets';
function block(s,start){let d=0;for(let i=start;i<s.length;i++){if(s[i]==='{')d++;else if(s[i]==='}'&&--d===0)return s.slice(start+1,i);}throw Error('Unbalanced graph');}
function graph(name){const source=fs.readFileSync(path.join(base,name+'.c'),'utf8'),m=[...source.matchAll(new RegExp('static void '+name+'\\(','g'))];if(m.length!==2)throw Error('Graph variants '+name);const fn=block(source,source.indexOf('{',m[1].index));let ops=block(fn,fn.indexOf('{',fn.indexOf('for ('))).replace(/WS\((?:is|os|rs),\s*(\d+)\)/g,'$1').replace(/\bE\b/g,'precise float');ops=ops.replace(/\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)/g,(_,op,a,b,c,d)=>'sum_products('+[a,b,c,d,op==='FNMS'?'true':'false'].join(',')+')');if(/\b(WS|stride|INT)\b/.test(ops))throw Error('Untranslated graph');const constants=[...fn.matchAll(/DK\((\w+),\s*([^;]+)\);/g)].map(m=>'const float '+m[1]+'='+m[2]+';').join('\n');return {source,body:constants+'\n'+ops};}
const small=graph('n1_4'),twiddle=graph('t1_12'),literals=[];function bits(v){const b=Buffer.alloc(4);b.writeFloatLE(v);return 'uintBitsToFloat(0x'+b.readUInt32LE().toString(16).padStart(8,'0')+'u)';}
for(let k=0;k<4;k++)for(let power=1;power<12;power++){const phase=k*power%48,angle=2*Math.PI*phase/48;let r=Math.cos(angle),i=Math.sin(angle);if(phase===0){r=1;i=0;}if(phase===12){r=0;i=1;}if(phase===24){r=-1;i=0;}if(phase===36){r=0;i=-1;}literals.push(bits(r),bits(i));}
const shader=`#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// FFTW3.3.10 n1_4+t1_12; copyright2003,2007-14 Matteo Frigo/MIT.
// Originalsources/COPYING retained. Fixed48 experimental operation graph.
layout(local_size_x=64)in;
layout(set=0,binding=0,std430)readonly buffer Input{vec2 v[];}src;
layout(set=0,binding=2,std430)writeonly buffer Output{vec2 v[];}dst;
layout(push_constant)uniform P{uint batches;uint inverse;uint b;uint c;}p;
float FMA(float a,float b,float c){precise float product=a*b;precise float r=product+c;return r;}
float FNMS(float a,float b,float c){precise float product=a*b;precise float r=c-product;return r;}
float FMS(float a,float b,float c){precise float product=a*b;precise float r=product-c;return r;}
float FNMA(float a,float b,float c){precise float product=a*b;precise float r=-(product+c);return r;}
float sum_products(float a,float b,float c,float d,bool minus){precise float t=a*b;precise float u=c*d;precise float r=minus?u-t:u+t;return r;}
void complex4(in float ri[4],in float ii[4],out float ro[4],out float io[4]){${small.body}}
void twiddle12(inout float ri[12],inout float ii[12],in float W[22]){${twiddle.body}}
const float table[88]=float[88](${literals.join(',')});
void main(){uint batch=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x;if(batch>=p.batches)return;float realPart[48],imagPart[48];
for(uint j=0;j<12;j++){float ri[4],ii[4],ro[4],io[4];for(uint l=0;l<4;l++){vec2 z=src.v[batch*48+j+12*l];ri[l]=p.inverse!=0?z.y:z.x;ii[l]=p.inverse!=0?z.x:z.y;}complex4(ri,ii,ro,io);for(uint k=0;k<4;k++){realPart[j+12*k]=ro[k];imagPart[j+12*k]=io[k];}}
for(uint k=0;k<4;k++){float ri[12],ii[12],W[22];for(uint j=0;j<12;j++){ri[j]=realPart[j+12*k];ii[j]=imagPart[j+12*k];}for(uint j=0;j<22;j++)W[j]=table[k*22+j];twiddle12(ri,ii,W);for(uint j=0;j<12;j++)dst.v[batch*48+k+4*j]=p.inverse!=0?vec2(ii[j],ri[j]):vec2(ri[j],ii[j]);}}
`;fs.mkdirSync(out);fs.writeFileSync(path.join(out,'fft48.comp'),shader);for(const [name,value]of [['n1_4',small],['t1_12',twiddle]])fs.writeFileSync(path.join(out,name+'.c'),value.source);fs.copyFileSync('tests/fftw_reference/COPYING',path.join(out,'COPYING'));fs.copyFileSync(__filename,path.join(out,'generator_snapshot.cjs'));
