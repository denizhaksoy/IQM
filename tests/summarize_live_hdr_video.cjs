// Acceptance requires every selected frame; adds subjective labels without fitting.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [root,dataset]=process.argv.slice(2);if(!dataset)throw Error('Usage: completed-video-root staged-dataset');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const plan=JSON.parse(fs.readFileSync(path.join(root,'plan.json'),'utf8'));
const score=JSON.parse(fs.readFileSync(path.join(root,'video_score.json'),'utf8'));
if(!score.complete||!score.exact||score.frames!==plan.indices.length||plan.indices.length!==Math.ceil(plan.decoded_frames/30))throw Error('Video evaluation incomplete');
for(const index of plan.indices){const base=path.join(root,'frame'+String(index).padStart(4,'0'));
 const r=JSON.parse(fs.readFileSync(path.join(base,'comparison.json'),'utf8'));
 if(r.zero_based_frame!==index||!r.exact||r.official.Q_hex!==r.cpp.Q_hex||r.official.Q_JOD_hex!==r.cpp.Q_JOD_hex)throw Error('Frame mismatch');
 if(sha(path.join(base,'decoded/reference.png'))!==r.reference_png_sha256||sha(path.join(base,'decoded/test.png'))!==r.test_png_sha256)throw Error('Decoded frame mutated');
 if(r.official.inputs[0].source_sha256!==r.reference_png_sha256||r.official.inputs[1].source_sha256!==r.test_png_sha256)throw Error('Reference input differs');
}
const sourceHashes={};function csv(name){const file=path.join(dataset,'metadata',name),text=fs.readFileSync(file,'utf8').replace(/^\uFEFF/,'').replace(/\r/g,'');sourceHashes[name]=sha(file);if(text.includes('"'))throw Error('Unsupported quoted CSV');const rows=text.trim().split('\n').map(l=>l.split(',')),header=rows.shift();return rows.map(r=>{if(r.length!==header.length)throw Error('CSV shape');return Object.fromEntries(header.map((h,i)=>[h,r[i]]));});}
function one(rows){const matches=rows.filter(r=>r.video===(plan.test_id||'4k_3M_Daylight1'));if(matches.length!==1)throw Error('Missing/duplicate subjective key');return matches[0];}
const metadata=one(csv('video_info_with_bitrate.csv'));if(metadata.ref!==(plan.reference_id||'4k_ref_Daylight1'))throw Error('Reference pairing differs');
const bright=one(csv('sureal_bright_mos_and_dmos.csv')),dark=one(csv('sureal_dark_mos_and_dmos.csv'));
const output=path.join(root,'acceptance.json');if(fs.existsSync(output))throw Error('Preserve prior acceptance');
fs.writeFileSync(output,JSON.stringify({scope:'one LIVE HDR video pair under documented local decoding/phase/backend choices; not full dataset or original challenge reproduction',exact_all_selected_frames:true,video_score:score,metadata,subjective_labels:{bright,dark},subjective_source_sha256:sourceHashes,correlation:{performed:false,reason:'one distorted video is insufficient for correlation analysis; bright/dark labels are not metric scores'}},null,2)+'\n');
console.log('Accepted all'+score.frames+' exact frames and video mean; subjective labels linked, correlation not performed');
