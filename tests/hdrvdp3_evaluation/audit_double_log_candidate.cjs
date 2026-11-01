const fs=require('fs'),p=require('path');const root=process.argv[2];if(!root)throw Error('Usage: candidate-root');
const read=f=>fs.readFileSync(p.join(root,f)),input=read('run/input.f64'),actual=read('run/actual.f64'),expected=read('run/expected.f64');
const count=input.length/8;if(count<663580||actual.length!==count*16||expected.length!==actual.length)throw Error('Query shape');
const asm=read('shader/multiply.spvasm').toString();if(/OpFConvert|OpTypeFloat 32/.test(asm)||!asm.includes('RoundingModeRTE 64')||!asm.includes('SignedZeroInfNanPreserve 64'))throw Error('Precision modes');
if(/VUID-|\[ERROR\]/.test(read('run.log').toString()))throw Error('Vulkan validation');
const ordered=b=>b>>63n?(~b)&((1n<<64n)-1n):b|(1n<<63n);
const stats=()=>({queries:0,bit_errors:0,max_ulp:0,nonfinite:0,histogram:{}}),total=[stats(),stats()];
const groups=read('run/groups.tsv').toString().trim().split(/\r?\n/).slice(1);let next=0;const reports=[];
for(const row of groups){const [name,startText,nText]=row.split('\t'),start=Number(startText),n=Number(nText);if(start!==next)throw Error('Group continuity');const group=[stats(),stats()];
 for(let i=start;i<start+n;i++){const x=input.readDoubleLE(i*8);if(!Number.isFinite(x)||x<=0)throw Error('Input domain');for(let op=0;op<2;op++){const offset=(2*i+op)*8,a=actual.readBigUInt64LE(offset),e=expected.readBigUInt64LE(offset),aa=ordered(a),ee=ordered(e),ulp=Number(aa>ee?aa-ee:ee-aa);for(const s of [group[op],total[op]]){s.queries++;s.bit_errors+=Number(a!==e);s.nonfinite+=Number(!Number.isFinite(actual.readDoubleLE(offset)));s.max_ulp=Math.max(s.max_ulp,ulp);const bin=ulp<=4?String(ulp):'>4';s.histogram[bin]=(s.histogram[bin]||0)+1;}}}
 reports.push({name,log:group[0],log10:group[1]});next+=n;
}
if(next!==count)throw Error('Group coverage');
const tsv=read('run/results.tsv').toString().trim().split(/\r?\n/).slice(1);for(let op=0;op<2;op++){const v=tsv[op].split('\t');if(Number(v[1])!==count||Number(v[2])!==total[op].bit_errors||Number(v[3])!==total[op].max_ulp||Number(v[4])!==total[op].nonfinite)throw Error('C++/Node audit disagreement');}
const result={scope:'Isolated Double logarithm candidate;CPU MSVC log/log10 oracle;no full-metric equivalence claim.',log:total[0],log10:total[1],corpus_exact:total.every(s=>s.bit_errors===0),production_accepted:false,groups:reports};
fs.writeFileSync(p.join(root,'audit_results.json'),JSON.stringify(result,null,2));console.log(JSON.stringify({log:total[0],log10:total[1],production_accepted:result.production_accepted}));
