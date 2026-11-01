const fs=require('fs'),p=require('path'),crypto=require('crypto');const root=process.argv[2],previous=process.argv[3];if(!root||!previous)throw Error('Usage: winlibm-root previous-log-root');
const read=f=>fs.readFileSync(p.join(root,f)),octave=read('octave_boundaries.f64'),reference=fs.readFileSync(p.join(previous,'octave_queries.f64'));
if(octave.length!==672036*16)throw Error('Octave shape');
const variants=['fma','sse','fma_boundaries','sse_boundaries'],results=[];
for(const v of variants){const expected=read(v+'/run/expected.f64'),actual=read(v+'/run/actual.f64'),input=read(v+'/run/input.f64'),oracle=v.includes('boundaries')?octave:reference,n=oracle.length/16;
 if(!expected.equals(oracle)||!actual.equals(oracle)||input.length!==n*8)throw Error('Query parity '+v);
 const first=read('fma_boundaries/run/input.f64');if(!first.subarray(0,input.length).equals(input))throw Error('Input lineage '+v);
 const audit=JSON.parse(read(v+'/audit_results.json'));if(!audit.corpus_exact||audit.production_accepted||audit.log.bit_errors||audit.log10.bit_errors)throw Error('Audit metadata '+v);
 if(/VUID-|\[ERROR\]/.test(read(v+'/run.log').toString()))throw Error('Validation '+v);
 results.push({variant:v,queries_per_operation:n,exact_output_words:n*2});
}
const manifest=[];for(const name of fs.readdirSync(p.join(root,'upstream')).filter(n=>n.endsWith('.asm'))){const bytes=read('upstream/'+name);if(!bytes.toString().includes('MIT License'))throw Error('Missing upstream license');manifest.push({name,sha256:crypto.createHash('sha256').update(bytes).digest('hex')});}
const commit=JSON.parse(read('upstream/commit.json')).sha;if(commit!=='6121d02aef0c775947d5dc655d7fc4ccfe2144ab')throw Error('Upstream commit');
fs.writeFileSync(p.join(root,'upstream_hashes.json'),JSON.stringify(manifest,null,2));
fs.writeFileSync(p.join(root,'results.json'),JSON.stringify({scope:'Isolated positive-normal Double log/log10 stage;exact against observed MSVC/Octave query arithmetic;not full metric integration.',upstream:'https://github.com/amd/win-libm/tree/'+commit,results,unique_input_words:672036,unique_exact_output_words:1344072,production_integrated:false},null,2));
fs.writeFileSync(p.join(root,'final_audit_PASS.txt'),'PASS:672036 positive-normal inputs;1344072 log/log10 outputs exact against CPU/Octave;FMA and SSE-order candidates both exact;earlier corpus overlaps;not production integrated.\n');console.log('PASS:1344072 unique Double log/log10 outputs exact;two source-order variants');
