const fs=require('fs'),path=require('path'),crypto=require('crypto');const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh output required');const source='validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/dft/scalar/codelets/n1_4.c',s=fs.readFileSync(source,'utf8'),matches=[...s.matchAll(/static void n1_4\(/g)];if(matches.length!==2)throw Error('Expected2scalar graphs');function body(text,start){let d=0;for(let i=start;i<text.length;i++){if(text[i]==='{')d++;else if(text[i]==='}'){if(--d===0)return text.slice(start+1,i);}}throw Error('Unbalanced');}
const fn=body(s,s.indexOf('{',matches[1].index));let ops=body(fn,fn.indexOf('{',fn.indexOf('for ('))).replace(/WS\((?:is|os),\s*(\d+)\)/g,'$1').replace(/\bE\b/g,'precise float');if(/\b(WS|stride)\b/.test(ops))throw Error('Untranslated source');const shader=`#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// Derived FFTW3.3.10 n1_4, copyright2003,2007-14 Matteo Frigo/MIT.
// Source/notices/COPYING retained; test-only translation.
layout(local_size_x=64)in;
layout(set=0,binding=0,std430)readonly buffer Input{vec2 v[];}src;
layout(set=0,binding=2,std430)writeonly buffer Output{vec2 v[];}dst;
layout(push_constant)uniform P{uint batches;uint inverse;uint b;uint c;}p;
void complex4(in float ri[4],in float ii[4],out float ro[4],out float io[4]){${ops}}
void main(){uint batch=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x;if(batch>=p.batches)return;float ri[4],ii[4],ro[4],io[4];for(uint i=0;i<4;i++){vec2 v=src.v[batch*4+i];ri[i]=p.inverse!=0?v.y:v.x;ii[i]=p.inverse!=0?v.x:v.y;}complex4(ri,ii,ro,io);for(uint i=0;i<4;i++)dst.v[batch*4+i]=p.inverse!=0?vec2(io[i],ro[i]):vec2(ro[i],io[i]);}
`;fs.mkdirSync(out);fs.writeFileSync(path.join(out,'n1_4.comp'),shader);fs.copyFileSync(source,path.join(out,'n1_4.c'));fs.copyFileSync('tests/fftw_reference/COPYING',path.join(out,'COPYING'));fs.writeFileSync(path.join(out,'derivation.json'),JSON.stringify({source,source_sha256:crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex'),scope:'ordinary n1_4 only, no4KFFTclaim'},null,2));
