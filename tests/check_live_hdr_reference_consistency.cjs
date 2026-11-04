// Exercise the guard with actual preserved reference records, without editing them.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {referenceConsistency}=require('./live_hdr_reference_consistency.cjs');
const [originalPath,repeat1Path,repeat2Path,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: original-score repeat1-score repeat2-score fresh-root');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8')),original=read(originalPath),r1=read(repeat1Path),r2=read(repeat2Path),checks=[];
assert(referenceConsistency([r1,r2,r1]).all_identical);checks.push('all three identical');
assert(!referenceConsistency([original,r1,r2]).all_identical);checks.push('historical outlier cannot be outvoted');
assert(!referenceConsistency([r1,original,r2]).all_identical);checks.push('outlier rejected independently of order');
assert(referenceConsistency([r1]).all_identical);checks.push('single-observation legacy profile supported');
assert.throws(()=>referenceConsistency([]));checks.push('empty evidence rejected');
for(const [label,mutate] of [
 ['backend mismatch',r=>r.backend='GPU'],
 ['input mismatch',r=>r.inputs[0].linear_sha256='changed'],
 ['invalid bits',r=>r.Q_hex='bad'],
 ['nonfinite value',r=>r.Q_hex='7fc00000']
]){const changed=structuredClone(r1);mutate(changed);assert.throws(()=>referenceConsistency([r1,changed]));checks.push(label+' rejected');}
fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'checks.json'),JSON.stringify({scope:'reference evidence consistency guards, not additional metric cases',passed:checks},null,2));console.log(JSON.stringify(checks));
