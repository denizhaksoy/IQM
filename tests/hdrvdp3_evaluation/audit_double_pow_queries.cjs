const fs=require('fs'),p=require('path');
const root=process.argv[2],native=process.argv[3];if(!root||!native)throw Error('Usage: probe-root native-cpu-root');
const records=JSON.parse(fs.readFileSync(p.join(root,'results.json')));
for(const op of ['sqrt','fma'])if(!records.find(r=>r.operation===op)?.validated)throw Error('Missing Double arithmetic control');
for(const op of ['log','log2','exp','exp2','pow']){const r=records.find(r=>r.operation===op);if(!r||r.compiler_exit!==2||r.validated)throw Error('Changed compiler capability; reassess diagnostic');}
const data=fs.readFileSync(p.join(root,'query_corpus/queries.f64'));
if(data.length!==1158704*24)throw Error('Query shape');
const rows=fs.readFileSync(p.join(root,'query_corpus/groups.tsv'),'utf8').trim().split(/\r?\n/).slice(1);let offset=0,checked=0,final=0;
for(const row of rows){const [name,start,size]=row.split('\t'),n=Number(size);if(Number(start)!==offset)throw Error('Group continuity');
 const field=name.endsWith('_band_diff')?'excitation':name.endsWith('_difference')?'psychometric':null;
 if(field){const gold=fs.readFileSync(p.join(native,name.replace(/_(band_diff|difference)$/,'_'+field)+'.f64'));if(gold.length!==n*8)throw Error('Native shape');
  for(let i=0;i<n;i++){const actual=data.readDoubleLE((offset+i)*24+16),expected=Math.abs(gold.readDoubleLE(i*8));if(!Object.is(actual,expected))throw Error('Native power differs');checked++;}
 }
 if(name.endsWith('_final_error')){const gold=fs.readFileSync(p.join(native,name.replace(/_final_error$/,'_error')+'.f64'));if(!data.subarray(offset*24+16,offset*24+24).equals(gold))throw Error('Native final error');final++;}
 offset+=n;
}
if(offset!==1158704||checked!==386208||final!==45)throw Error('Coverage');
const result={query_triples:offset,native_excitation_psychometric_power_values_checked:checked,native_final_errors_checked:final,scope:'CPU/UCRT pow oracle corpus;compiler capability measurement only. No GPU transcendental numerical evaluation performed.'};
fs.writeFileSync(p.join(root,'corpus_audit.json'),JSON.stringify(result,null,2));console.log('PASS:1158704 CPU pow queries;386208 native power results and45 final errors exact');
