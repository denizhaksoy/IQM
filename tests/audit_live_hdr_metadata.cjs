// Read-only metadata inspection. Does not decode, sample or score videos.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const [root, output] = process.argv.slice(2);
if (!root || !output || fs.existsSync(output)) throw Error('Specify dataset root and fresh output directory');
const source_sha256 = {};
function csv(name) {
  const bytes = fs.readFileSync(path.join(root, 'metadata', name));
  source_sha256[name] = crypto.createHash('sha256').update(bytes).digest('hex');
  // These supplied metadata files have no quoted fields; reject rather than misparse.
  const text = bytes.toString('utf8').replace(/^\uFEFF/, '').replace(/\r/g, '');
  if (text.includes('"')) throw Error('Quoted CSV requires a full CSV parser');
  const lines = text.trim().split('\n').map(s => s.split(','));
  const header = lines.shift();
  return lines.map(fields => {
    if (fields.length !== header.length) throw Error('Malformed CSV row');
    return Object.fromEntries(header.map((h,i) => [h,fields[i]]));
  });
}
const rows = csv('video_info_with_bitrate.csv');
const names = new Set(rows.map(r => r.video));
const refs = new Set(rows.map(r => r.ref));
const count = values => values.reduce((a,v) => (a[v]=(a[v]||0)+1,a), {});
const duplicates = values => Object.entries(count(values)).filter(([,n])=>n!==1).map(([k])=>k);
const absent = (a,b) => [...a].filter(v => !b.has(v)).sort();
const inventory = JSON.parse(fs.readFileSync(path.join(root,'metadata/videos.json'),'utf8').replace(/^\uFEFF/,''));
const archive = new Set(inventory.map(v=>path.posix.basename(v.FullName,'.mp4')));
const labels = {};
for (const condition of ['bright','dark']) {
  const table = csv(`sureal_${condition}_mos_and_dmos.csv`);
  const keys = new Set(table.map(r=>r.video));
  labels[condition] = {rows:table.length, duplicates:duplicates(table.map(r=>r.video)),
    missing:absent(names,keys), extra:absent(keys,names), invalid:[]};
  for (const r of table) for (const suffix of ['mos','mos_ci95','dmos']) {
    const field = `${condition}_${suffix}`;
    if (!r[field]?.trim() || !Number.isFinite(Number(r[field]))) labels[condition].invalid.push([r.video,field]);
  }
}
const report = {scope:'Metadata and archive inventory only; not full video decode verification',source_sha256,
  rows:rows.length,unique_videos:names.size,unique_references:refs.size,
  self_reference_rows:rows.filter(r=>r.video===r.ref).length,
  distorted_rows:rows.filter(r=>r.video!==r.ref).length,
  duplicate_videos:duplicates(rows.map(r=>r.video)),missing_archive_videos:absent(names,archive),
  missing_archive_references:absent(refs,archive),
  frame_count_mismatches:rows.filter(r=>r.frm_cnt!==r.ref_frm_cnt).map(r=>r.video),
  reference_group_sizes:count(rows.map(r=>r.ref)),labels};
fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'metadata_audit.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
