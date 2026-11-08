const fs=require('fs'),path=require('path'),crypto=require('crypto');const [out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Fresh output required');const source='validation/fftw_4k_sources_20261107_run01/fftw-3.3.10/rdft/scalar/r2cf/r2cf_6.c',s=fs.readFileSync(source,'utf8');const matches=[...s.matchAll(/static void r2cf_6\(/g)];if(matches.length!==2)throw Error('Expected scalar graph variants');// Extract balanced blocks without modifying the original source.
function block(text,start){let depth=0;for(let i=start;i<text.length;i++){if(text[i]==='{')depth++;if(text[i]==='}'){depth--;if(depth===0)return text.slice(start+1,i);}}throw Error('Unbalanced block');}
const fn=block(s,s.indexOf('{',matches[1].index)),constants=[...fn.matchAll(/DK\((\w+),\s*([^;]+)\);/g)].map(m=>'const float '+m[1]+'='+m[2]+';').join('\n');const start=fn.indexOf('for (');let ops=block(fn,fn.indexOf('{',start)).replace(/WS\((?:rs|csr|csi),\s*(\d+)\)/g,'$1').replace(/R([01])\[(\d+)\]/g,(_,a,b)=>'samples['+(2*Number(b)+Number(a))+']').replace(/\bE\b/g,'precise float');if(/\b(WS|R0|R1|stride)\b/.test(ops))throw Error('Untranslated source');const shader=`#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// Derived from FFTW3.3.10 r2cf_6, copyright2003,2007-14 Matteo Frigo/MIT.
// Original source/notices/COPYING retained in source artifact; test-only.
layout(local_size_x=64) in;
layout(set=0,binding=0,std430) readonly buffer Input{float v[];}src;
layout(set=0,binding=2,std430) writeonly buffer Output{vec2 v[];}dst;
layout(push_constant) uniform P{uint batches;uint a;uint b;uint c;}p;
float FNMS(float a,float b,float c){precise float product=a*b;return c-product;}
void real6(in float samples[6],out float Cr[4],out float Ci[4]){
${constants}
for(int i=0;i<4;i++){Cr[i]=0;Ci[i]=0;}
${ops}
}
void main(){uint batch=(gl_WorkGroupID.y*gl_NumWorkGroups.x+gl_WorkGroupID.x)*64+gl_LocalInvocationID.x;if(batch>=p.batches)return;float samples[6],r[4],im[4];for(uint i=0;i<6;i++)samples[i]=src.v[batch*6+i];real6(samples,r,im);for(uint i=0;i<4;i++)dst.v[batch*4+i]=vec2(r[i],im[i]);}
`;fs.mkdirSync(out);fs.writeFileSync(path.join(out,'r2cf6.comp'),shader);fs.copyFileSync(source,path.join(out,'r2cf_6.c'));fs.copyFileSync('tests/fftw_reference/COPYING',path.join(out,'COPYING'));fs.writeFileSync(path.join(out,'derivation.json'),JSON.stringify({source,source_sha256:crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex'),scope:'ordinary r2cf6 graph only, no4KFFTclaim'},null,2));
