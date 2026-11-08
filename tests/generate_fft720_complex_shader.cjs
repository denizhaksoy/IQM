const fs=require('fs'),path=require('path');const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh output required');const source='validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/dft/simd/common/t1fv_15.c',text=fs.readFileSync(source,'utf8'),matches=[...text.matchAll(/static void t1fv_15\(/g)];if(matches.length!==2)throw Error('Graph variants');function block(s,start){let d=0;for(let i=start;i<s.length;i++){if(s[i]==='{')d++;else if(s[i]==='}'&&--d===0)return s.slice(start+1,i);}throw Error('Unbalanced');}const fn=block(text,text.indexOf('{',matches[1].index));let ops=block(fn,fn.indexOf('{',fn.indexOf('for ('))).replace(/\bV\b/g,'precise vec2').replace(/BYTWJ\(&\(W\[TWVL \* (\d+)\]\), (\w+)\)/g,(_,n,v)=>'VZMULJ(W['+(Number(n)/2)+'],'+v+')').replace(/BYTWJ\(&\(W\[0\]\), (\w+)\)/g,'VZMULJ(W[0],$1)').replace(/WS\(rs,\s*(\d+)\)/g,'$1').replace(/LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)/g,'x[$1]').replace(/ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);/g,'x[$1]=$2;').replace(/LDK\((\w+)\)/g,'vec2($1)');if(/\b(LD|ST|WS|TWVL|BYTWJ|INT|VLEAVE)\b/.test(ops))throw Error('Untranslated SIMD operation');// Match six FMA contractions observed in the frozen official FFTW DLL at RVA 0x284dd0.
// Original uncontracted graph and failed comparisons remain in run01 artifacts.
for(const [a,b] of [
 ['TN = VADD(TI, TH);','TN = VFMA(vec2(KP559016994), VSUB(To, TF), TH);'],
 ['TJ = VSUB(TH, TI);','TJ = VFNMS(vec2(KP559016994), VSUB(To, TF), TH);'],
 ['T1h = VSUB(T1d, T1g);','T1h = VFMS(vec2(KP484122918), VSUB(T1b, T1c), T1g);'],
 ['T1o = VADD(T1d, T1g);','T1o = VFMA(vec2(KP484122918), VSUB(T1b, T1c), T1g);'],
 ['TZ = VSUB(TX, TY);','TZ = VFNMS(vec2(KP559016994), VSUB(TS, TV), TX);'],
 ['T1l = VADD(TY, TX);','T1l = VFMA(vec2(KP559016994), VSUB(TS, TV), TX);']
]){if(!ops.includes(a))throw Error('Missing contraction '+a);ops=ops.replace(a,b);}
const constants=[...fn.matchAll(/DVK\((\w+),\s*([^;]+)\);/g)].map(m=>'const float '+m[1]+'='+m[2]+';').join('\n');let shader=fs.readFileSync('validation/fft48_shader_20261107_run01/fft48.comp','utf8');shader=shader.slice(0,shader.indexOf('void main()'));shader+=`
vec2 VADD(vec2 a,vec2 b){precise vec2 r=a+b;return r;}
vec2 VSUB(vec2 a,vec2 b){precise vec2 r=a-b;return r;}
vec2 VMUL(vec2 a,vec2 b){precise vec2 r=a*b;return r;}
vec2 VBYI(vec2 a){return vec2(-a.y,a.x);}
vec2 VFMS(vec2 a,vec2 b,vec2 c){return fma(a,b,-c);}
vec2 VFMA(vec2 a,vec2 b,vec2 c){return fma(a,b,c);}
vec2 VFNMS(vec2 a,vec2 b,vec2 c){return fma(-a,b,c);}
vec2 VZMULJ(vec2 a,vec2 b){precise vec2 t=a.y*b.yx;return vec2(fma(a.x,b.x,t.x),fma(a.x,b.y,-t.y));}
void twiddle15(inout vec2 x[15],in vec2 W[14]){${constants}\n${ops}}
void complex48(in vec2 inputValue[48],out vec2 outputValue[48]){float re[48],im[48];for(uint j=0;j<12;j++){float ri[4],ii[4],ro[4],io[4];for(uint l=0;l<4;l++){vec2 z=inputValue[j+12*l];ri[l]=z.x;ii[l]=z.y;}complex4(ri,ii,ro,io);for(uint k=0;k<4;k++){re[j+12*k]=ro[k];im[j+12*k]=io[k];}}for(uint k=0;k<4;k++){float ri[12],ii[12],W[22];for(uint j=0;j<12;j++){ri[j]=re[j+12*k];ii[j]=im[j+12*k];}for(uint j=0;j<22;j++)W[j]=table[k*22+j];twiddle12(ri,ii,W);for(uint j=0;j<12;j++)outputValue[k+4*j]=vec2(ri[j],ii[j]);}}
`;function bits(x){const b=Buffer.alloc(4);b.writeFloatLE(x);return 'uintBitsToFloat(0x'+b.readUInt32LE().toString(16).padStart(8,'0')+'u)';}const weights=[];for(let k=0;k<48;k++)for(let power=1;power<15;power++){const phase=k*power%720,a=2*Math.PI*phase/720;let r=Math.cos(a),i=Math.sin(a);if(phase===0){r=1;i=0;}if(phase===180){r=0;i=1;}if(phase===360){r=-1;i=0;}if(phase===540){r=0;i=-1;}weights.push('vec2('+bits(r)+','+bits(i)+')');}shader+=`const vec2 table720[672]=vec2[672](${weights.join(',')});
void main(){uint batch=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x;if(batch>=p.batches)return;vec2 intermediate[720];for(uint j=0;j<15;j++){vec2 x[48],y[48];for(uint l=0;l<48;l++){vec2 z=src.v[batch*720+j+15*l];x[l]=p.inverse!=0?z.yx:z;}complex48(x,y);for(uint k=0;k<48;k++)intermediate[j+15*k]=y[k];}for(uint k=0;k<48;k++){vec2 x[15],W[14];for(uint j=0;j<15;j++)x[j]=intermediate[j+15*k];for(uint j=0;j<14;j++)W[j]=table720[k*14+j];twiddle15(x,W);for(uint j=0;j<15;j++)dst.v[batch*720+k+48*j]=p.inverse!=0?x[j].yx:x[j];}}
`;fs.mkdirSync(out);fs.writeFileSync(path.join(out,'fft720.comp'),shader);fs.copyFileSync(source,path.join(out,'t1fv_15.c'));fs.copyFileSync('tests/fftw_reference/COPYING',path.join(out,'COPYING'));fs.copyFileSync(__filename,path.join(out,'generator_snapshot.cjs'));
