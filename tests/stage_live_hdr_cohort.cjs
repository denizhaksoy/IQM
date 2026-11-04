// Materialize only the declared subset; all existing files must match ZIP entries.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const [archive,dataset,cohortPath,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: archive.zip staged-dataset cohort.json fresh-report-root');
const workspace=fs.realpathSync(process.cwd()),destination=path.resolve(dataset,'sample_pair');
if(!destination.toLowerCase().startsWith(workspace.toLowerCase()+path.sep))throw Error('Destination outside workspace');
const cohort=JSON.parse(fs.readFileSync(cohortPath,'utf8')),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const metaFile=path.join(dataset,'metadata/video_info_with_bitrate.csv');if(sha(metaFile)!==cohort.source_csv_sha256['video_info_with_bitrate.csv'])throw Error('Metadata differs');
const text=fs.readFileSync(metaFile,'utf8').replace(/^\uFEFF/,'').replace(/\r/g,'');if(text.includes('"'))throw Error('Unsupported CSV quoting');
const lines=text.trim().split('\n').map(l=>l.split(',')),header=lines.shift(),metadata=new Map(lines.map(row=>{if(row.length!==header.length)throw Error('CSV shape');const r=Object.fromEntries(header.map((h,i)=>[h,row[i]]));return[r.video,r];}));
const identifiers=[...new Set(cohort.cases.flatMap(c=>{if(metadata.get(c.video)?.ref!==c.reference)throw Error('Pairing differs');return[c.video,c.reference];}))].sort();
if(identifiers.some(id=>!/^[A-Za-z0-9_.-]+$/.test(id)))throw Error('Invalid identifiers');
fs.mkdirSync(out,{recursive:true});const spec=path.resolve(out,'extraction_spec.json');fs.writeFileSync(spec,JSON.stringify({workspace,destination,archive:path.resolve(archive),identifiers,extraction_report:path.resolve(out,'extraction.json')}));
const script=fs.readFileSync('tests/stage_live_hdr_cohort.ps1','utf8');
const command='& {\n'+script+'\n} -Spec \''+spec.replace(/'/g,"''")+'\'';
const run=cp.spawnSync('powershell.exe',['-NoProfile','-NonInteractive','-EncodedCommand',Buffer.from(command,'utf16le').toString('base64')],{encoding:'utf8',windowsHide:true,maxBuffer:8*1024*1024});
fs.writeFileSync(path.join(out,'extraction_stdout.log'),run.stdout||'');fs.writeFileSync(path.join(out,'extraction_stderr.log'),run.stderr||'');if(run.error)throw run.error;if(run.status!==0)throw Error('Extraction/identity verification failed; inspect logs');
const records=[];
for(const id of identifiers){const file=path.join(destination,id+'.mp4'),args=['-v','error','-select_streams','v:0','-show_streams','-of','json',file];const p=cp.spawnSync('ffprobe',args,{encoding:'utf8',windowsHide:true});if(p.status!==0)throw Error('Probe failed: '+id);fs.writeFileSync(path.join(out,id+'.ffprobe.json'),p.stdout);const s=JSON.parse(p.stdout).streams[0],m=metadata.get(id);
 if(s.color_transfer!=='smpte2084'||s.color_primaries!=='bt2020'||s.color_space!=='bt2020nc'||s.pix_fmt!=='yuv420p10le'||s.width!==Number(m.w)||s.height!==Number(m.h)||Number(s.nb_frames)!==Number(m.frm_cnt))throw Error('Stream/header metadata mismatch: '+id);
 records.push({video:id,width:s.width,height:s.height,header_frame_count:Number(s.nb_frames),actual_r_frame_rate:s.r_frame_rate,metadata_fps:m.fps,color_range:s.color_range||null,transfer:s.color_transfer,primaries:s.color_primaries,matrix:s.color_space,pixel_format:s.pix_fmt});
}
fs.writeFileSync(path.join(out,'inventory.json'),JSON.stringify({scope:'declared archive subset: byte identity and stream headers only; not full decode/timestamp or quality validation',author_cohort_verified:cohort.original_train_video_info_csv_verified===true,cohort_sha256:sha(cohortPath),videos:records.length,distorted_pairs:cohort.cases.length,records},null,2)+'\n');console.log(records.length+' videos staged/ZIP-byte-verified and stream headers checked; quality cohort remains incomplete');
