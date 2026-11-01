const fs=require('fs'),p=require('path');const [upstream,out,variant]=process.argv.slice(2);if(!upstream||!out||!['fma','sse'].includes(variant)||fs.existsSync(out))throw Error('Usage: upstream fresh.comp fma|sse');
const read=f=>fs.readFileSync(p.join(upstream,f),'utf8'),log=read('log.asm'),decimal=read('log10.asm');
const bits=h=>{h=h.replace(/^0+(?=[0-9a-f]{16}$)/i,'').padStart(16,'0');if(h.length!==16)throw Error('Hex shape');return `uvec2(0x${h.slice(8)}u,0x${h.slice(0,8)}u)`;};
const constant=(source,name)=>{const m=source.match(new RegExp('^'+name+'\\s+DQ\\s+([0-9a-f]+)h','im'));if(!m)throw Error(name);return `packDouble2x32(${bits(m[1])})`;};
const tables=[['inverse','log_F_inv_qword_table.asm','__log_F_inv_qword'],['lnHead','log_256_lead_tail_table.asm','__log_256_lead'],['lnTail','log_256_lead_tail_table.asm','__log_256_tail'],['lgHead','log10_256_lead_tail_table.asm','__log10_256_lead'],['lgTail','log10_256_lead_tail_table.asm','__log10_256_tail']];
let source='/*\n'+log.split('\n').slice(0,24).map(s=>s.replace(/^; ?/,'')).join('\n')+'\nAdapted from https://github.com/amd/win-libm; diagnostic positive-normal domain only.\n*/\n#version 450\n#extension GL_ARB_gpu_shader_fp64 : require\nlayout(local_size_x=64) in;\nlayout(set=0,binding=0,std430) readonly buffer Input {double v[];} src;\nlayout(set=0,binding=1,std430) writeonly buffer Output {double v[];} dst;\nlayout(push_constant) uniform Push {uint count;uint a;uint b;uint c;} p;\n';
for(const [name,file,label]of tables){const s=read(file),start=s.search(new RegExp('^'+label+'(?:\\s+DQ|:)','m'));if(start<0)throw Error('Table label');const values=[];for(const line of s.slice(start).split(/\r?\n/)){if(values.length&&/^\w/.test(line))break;const m=line.match(/\bDQ\s+([0-9a-f]+)h/i);if(m)values.push(bits(m[1]));}if(values.length===258&&values[257]===bits('0'))values.pop();if(values.length!==257)throw Error(name+' table size '+values.length);source+=`const uvec2 ${name}[257]=uvec2[257](${values.join(',')});\n`;}
const fma=variant==='fma';
source+=`
double divideCorrected(double a,double b){precise double q=a/b;precise double r=fma(-q,b,a);precise double c=r/b;precise double result=q+c;return result;}
double nearOne(double x,bool base10){
 precise double r=x-1.0lf,den=2.0lf+r;double halfU=divideCorrected(r,den);
 precise double correction=r*halfU,u=halfU+halfU,u2=u*u;
 precise double a=${fma?`fma(${constant(log,'__real_ca2')},u2,${constant(log,'__real_ca1')})`:`${constant(log,'__real_ca2')}*u2+${constant(log,'__real_ca1')}`};
 precise double b=${fma?`fma(${constant(log,'__real_ca4')},u2,${constant(log,'__real_ca3')})`:`${constant(log,'__real_ca4')}*u2+${constant(log,'__real_ca3')}`};
 precise double u3=u2*u,first=a*u3,six=u3*u3,seven=six*u;
 precise double poly;
 if(base10){precise double second=b*seven;poly=first+second;}else{poly=${fma?'fma(b,seven,first)':'first+b*seven'};}
 precise double tail=poly-correction;
 if(!base10){precise double result=r+tail;return result;}
 uvec2 rb=unpackDouble2x32(r);double head=packDouble2x32(uvec2(0u,rb.y));precise double low=r-head,small=tail+low;
 precise double a1=small*${constant(decimal,'__real_log10_e_lead')},a2=small*${constant(decimal,'__real_log10_e_tail')},a3=head*${constant(decimal,'__real_log10_e_tail')},a4=head*${constant(decimal,'__real_log10_e_lead')};
 precise double s1=a3+a2,s2=s1+a1,result=s2+a4;return result;
}
double candidate(double x,bool base10){
 precise double delta=x-1.0lf;if(abs(delta)<0.0625lf)return nearOne(x,base10);
 uvec2 bits=unpackDouble2x32(x);int exponent=int((bits.y>>20u)&2047u)-1023;
 uint rounded=(bits.y&0x000ff000u)+((bits.y&0x00000800u)<<1u),index=rounded>>12u;
 double y=packDouble2x32(uvec2(bits.x,(bits.y&0x000fffffu)|0x3fe00000u)),center=packDouble2x32(uvec2(0u,rounded|0x3fe00000u));
 precise double difference=center-y,r=difference*packDouble2x32(inverse[index]),r2=r*r,r4=r2*r2;
 precise double a=${fma?'fma(1.0lf/6.0lf,r,1.0lf/5.0lf)':'(1.0lf/6.0lf)*r+1.0lf/5.0lf'};
 precise double b=${fma?'fma(1.0lf/3.0lf,r,0.5lf)':'(1.0lf/3.0lf)*r+0.5lf'};
 precise double c=${fma?'fma(a,r,0.25lf)':'a*r+0.25lf'};
 precise double d=${fma?'fma(b,r2,r)':'b*r2+r'};
 precise double poly=${fma?'fma(c,r4,d)':'d+c*r4'};
 if(base10)poly=poly*${constant(decimal,'__real_log10_e')};
 double lead=base10?${constant(decimal,'__real_log10_2_lead')}:${constant(log,'__real_log2_lead')};
 double tail=base10?${constant(decimal,'__real_log10_2_tail')}:${constant(log,'__real_log2_tail')};
 double tableHead=packDouble2x32(base10?lgHead[index]:lnHead[index]),tableTail=packDouble2x32(base10?lgTail[index]:lnTail[index]);
 precise double low=${fma?'fma(tail,double(exponent),-poly)':'tail*double(exponent)-poly'};
 precise double t=tableTail+low,high=${fma?'fma(double(exponent),lead,tableHead)':'tableHead+lead*double(exponent)'};
 precise double result=high+t;return result;
}
void main(){uint i=gl_GlobalInvocationID.x;if(i>=p.count)return;dst.v[2u*i]=candidate(src.v[i],false);dst.v[2u*i+1u]=candidate(src.v[i],true);}
`;
fs.writeFileSync(out,source);console.log('Generated '+variant+' candidate with five257-entry upstream tables');
