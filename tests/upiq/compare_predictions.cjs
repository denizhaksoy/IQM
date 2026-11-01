// Compare implementation predictions with independently generated reference
// predictions. Subjective labels are deliberately not numeric equality targets.
const fs=require('fs'),p=require('path');const [referenceFile,actualFile,out]=process.argv.slice(2);if(!out||fs.existsSync(out))throw Error('Usage: reference.json actual.json fresh-output');
const reference=JSON.parse(fs.readFileSync(referenceFile)),actual=JSON.parse(fs.readFileSync(actualFile));
if(!Array.isArray(reference.cases)||!reference.cases.length||!Array.isArray(actual.cases))throw Error('Nonempty cases arrays required');
fs.mkdirSync(out,{recursive:true});
const asBits=(r,field)=>{const s=r[field+'_text'];if(typeof s!=='string'||!s.trim()||!Number.isFinite(Number(s)))throw Error('17-digit score text required');const b=Buffer.alloc(8);b.writeDoubleBE(Number(s));return b.readBigUInt64BE();};
const declaredBits=(r,field)=>{if(typeof r[field+'_hex']!=='string')throw Error('Reference score hex required');const bytes=Buffer.from(r[field+'_hex'],'hex');const b=Buffer.alloc(8);if(bytes.length===8)b.writeDoubleBE(bytes.readDoubleBE());else if(bytes.length===4)b.writeDoubleBE(bytes.readFloatBE());else throw Error('Invalid native score type');if(b.readBigUInt64BE()!==asBits(r,field))throw Error('Reference decimal/hex disagreement');return b.readBigUInt64BE();};
const ordered=x=>x>>63n?(~x)&((1n<<64n)-1n):x|(1n<<63n);
function index(rows){const map=new Map();for(const r of rows){if(typeof r.id!=='string'||map.has(r.id))throw Error('Missing/duplicate id');map.set(r.id,r);}return map;}
const gold=index(reference.cases),got=index(actual.cases),records=[];let errors=0,missing=0,protocolErrors=0,maxUlp=0;
const keys=['reference_sha256','test_sha256','height','width','precision','encoding','ppd','observer_age','rgb_display'];
for(const [id,r]of gold){const a=got.get(id);if(!a){missing++;continue;}let protocol=true;for(const key of keys){if(r[key]===undefined||a[key]!==r[key])protocol=false;}if(!protocol){protocolErrors++;continue;}
 const row={id};for(const field of ['Q','Q_JOD']){const e=declaredBits(r,field),v=asBits(a,field),eo=ordered(e),vo=ordered(v),ulp=Number(eo>vo?eo-vo:vo-eo);if(e!==v)errors++;maxUlp=Math.max(maxUlp,ulp);row[field]={reference:r[field+'_text'],actual:a[field+'_text'],bit_equal:e===v,ulp_difference:ulp};}records.push(row);
}
const unexpected=[...got.keys()].filter(id=>!gold.has(id)),pass=!errors&&!missing&&!protocolErrors&&!unexpected.length;
const result={dataset_status:reference.dataset_status,reference_cases:gold.size,actual_cases:got.size,compared_cases:records.length,missing_cases:missing,unexpected_cases:unexpected,protocol_mismatches:protocolErrors,score_bit_mismatches:errors,max_ulp:maxUlp,exact_parity:pass,scope:'Recorded cases only;not a UPIQ acceptance claim unless both files actually contain the complete verified dataset.',records};
fs.writeFileSync(p.join(out,'comparison.json'),JSON.stringify(result,null,2));console.log(JSON.stringify({reference_cases:gold.size,compared_cases:records.length,missing_cases:missing,unexpected_cases:unexpected.length,protocol_mismatches:protocolErrors,score_bit_mismatches:errors,exact_parity:pass}));process.exitCode=pass?0:2;
