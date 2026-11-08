const fs=require('fs'),path=require('path');const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh output required');const source='validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/dft/simd/common/t3fv_16.c',s=fs.readFileSync(source,'utf8'),matches=[...s.matchAll(/static void t3fv_16\(/g)];function block(s,p){let d=0;for(let i=p;i<s.length;i++){if(s[i]==='{')d++;if(s[i]==='}'&&--d===0)return s.slice(p+1,i);}throw Error('Unbalanced graph');}if(matches.length!==2)throw Error('Variants');const fn=block(s,s.indexOf('{',matches[1].index));let ops=block(fn,fn.indexOf('{',fn.indexOf('for ('))).replace(/\bV\b/g,'precise vec2').replace(/LDW\(&\(W\[TWVL \* (\d+)\]\)\)/g,(_,n)=>'W['+(Number(n)/2)+']').replace(/LDW\(&\(W\[0\]\)\)/g,'W[0]').replace(/BYTWJ\(&\(W\[TWVL \* (\d+)\]\), (\w+)\)/g,(_,n,v)=>'VZMULJ(W['+(Number(n)/2)+'],'+v+')').replace(/BYTWJ\(&\(W\[0\]\), (\w+)\)/g,'VZMULJ(W[0],$1)').replace(/WS\(rs,\s*(\d+)\)/g,'$1').replace(/LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)/g,'x[$1]').replace(/ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);/g,'x[$1]=$2;').replace(/LDK\((\w+)\)/g,'vec2($1)');if(/\b(LD|ST|WS|TWVL|BYTWJ|INT|VLEAVE)\b/.test(ops))throw Error('Untranslated graph');// Eight contractions observed in official DLL native t3fv16 kernel.
ops='precise vec2 TLarg,TZarg,T1oarg,T1targ;\n'+ops;
for(const [a,b] of [
 ['TL = VMUL(vec2(KP707106781), VSUB(TE, TK));','TLarg = VSUB(TE, TK); TL = VMUL(vec2(KP707106781), TLarg);'],
 ['TZ = VMUL(vec2(KP707106781), VADD(TK, TE));','TZarg = VADD(TK, TE); TZ = VMUL(vec2(KP707106781), TZarg);'],
 ['T1o = VMUL(vec2(KP707106781), VADD(T1k, T1n));','T1oarg = VADD(T1k, T1n); T1o = VMUL(vec2(KP707106781), T1oarg);'],
 ['T1t = VMUL(vec2(KP707106781), VSUB(T1n, T1k));','T1targ = VSUB(T1n, T1k); T1t = VMUL(vec2(KP707106781), T1targ);'],
 ['T1p = VADD(T1h, T1o);','T1p = VFMA(vec2(KP707106781), T1oarg, T1h);'],
 ['T1v = VSUB(T1h, T1o);','T1v = VFNMS(vec2(KP707106781), T1oarg, T1h);'],
 ['T1u = VBYI(VADD(T1s, T1t));','T1u = VBYI(VFMA(vec2(KP707106781), T1targ, T1s));'],
 ['T1w = VBYI(VSUB(T1t, T1s));','T1w = VBYI(VFMS(vec2(KP707106781), T1targ, T1s));'],
 ['TS = VSUB(TL, TR);','TS = VFMS(vec2(KP707106781), TLarg, TR);'],
 ['T1a = VADD(TR, TL);','T1a = VFMA(vec2(KP707106781), TLarg, TR);'],
 ['T10 = VADD(TY, TZ);','T10 = VFMA(vec2(KP707106781), TZarg, TY);'],
 ['T17 = VSUB(TY, TZ);','T17 = VFNMS(vec2(KP707106781), TZarg, TY);']
]){if(!ops.includes(a))throw Error('Missing contraction '+a);ops=ops.replace(a,b);}
const constants=[...fn.matchAll(/DVK\((\w+),\s*([^;]+)\);/g)].map(m=>'const float '+m[1]+'='+m[2]+';').join('\n');function bits(v){let b=Buffer.alloc(4);b.writeFloatLE(v);return 'uintBitsToFloat(0x'+b.readUInt32LE().toString(16).padStart(8,'0')+'u)';}let weights=[];for(let k=0;k<24;k++)for(let j of [1,3,9,15]){let phase=k*j%384,t=2*Math.PI*phase/384,r=Math.cos(t),im=Math.sin(t);if(phase===0)[r,im]=[1,0];if(phase===96)[r,im]=[0,1];if(phase===192)[r,im]=[-1,0];weights.push('vec2('+bits(r)+','+bits(im)+')');}const shader=`#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// FFTW 3.3.10 t3fv_16; copyright2003,2007-14 Frigo/MIT. Source/COPYING retained.
layout(local_size_x=64)in;
layout(set=0,binding=0,std430)readonly buffer Input{vec2 v[];}src;
layout(set=0,binding=2,std430)writeonly buffer Output{vec2 v[];}dst;
layout(push_constant)uniform P{uint batches;uint a;uint b;uint c;}p;
vec2 VFMS(vec2 a,vec2 b,vec2 c){return fma(a,b,-c);}
vec2 VFMA(vec2 a,vec2 b,vec2 c){return fma(a,b,c);}
vec2 VFMAI(vec2 b,vec2 c){precise vec2 r=vec2(c.x-b.y,c.y+b.x);return r;}
vec2 VFNMSI(vec2 b,vec2 c){precise vec2 r=vec2(c.x+b.y,c.y-b.x);return r;}
vec2 VZMUL(vec2 a,vec2 b){precise vec2 t=a.y*b.yx;return vec2(fma(a.x,b.x,-t.x),fma(a.x,b.y,t.y));}
vec2 VADD(vec2 a,vec2 b){precise vec2 r=a+b;return r;}vec2 VSUB(vec2 a,vec2 b){precise vec2 r=a-b;return r;}vec2 VMUL(vec2 a,vec2 b){precise vec2 r=a*b;return r;}vec2 VBYI(vec2 a){return vec2(-a.y,a.x);}vec2 VFNMS(vec2 a,vec2 b,vec2 c){return fma(-a,b,c);}vec2 VZMULJ(vec2 a,vec2 b){precise vec2 t=a.y*b.yx;return vec2(fma(a.x,b.x,t.x),fma(a.x,b.y,-t.y));}
void twiddle(inout vec2 x[16],in vec2 W[4]){${constants}\n${ops}}
const vec2 table[96]=vec2[96](${weights.join(',')});
void main(){uint batch=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x;if(batch>=p.batches)return;for(uint k=0;k<24;k++){vec2 x[16],W[4];for(uint j=0;j<16;j++)x[j]=src.v[batch*384+j*24+k];for(uint j=0;j<4;j++)W[j]=table[k*4+j];twiddle(x,W);for(uint j=0;j<16;j++)dst.v[batch*384+j*24+k]=x[j];}}
`;fs.mkdirSync(out);fs.writeFileSync(path.join(out,'t3fv16.comp'),shader);fs.copyFileSync(source,path.join(out,'t3fv_16.c'));fs.copyFileSync('tests/fftw_reference/COPYING',path.join(out,'COPYING'));fs.copyFileSync(__filename,path.join(out,'generator_snapshot.cjs'));
