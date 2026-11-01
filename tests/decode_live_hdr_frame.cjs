// Explicit local RGB48 diagnostic decode, not a claim about the author's PNG depth.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const [reference,test,indexText,out]=process.argv.slice(2);const index=Number(indexText);
if(!out||!Number.isSafeInteger(index)||index<0||fs.existsSync(out))throw Error('Usage: reference.mp4 test.mp4 zero-based-index fresh-root');
fs.mkdirSync(out,{recursive:true});
const fingerprint=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const records=[];
for(const [side,input] of [['reference',reference],['test',test]]){
 const probeArgs=['-v','error','-select_streams','v:0','-show_streams','-of','json',input];
 const probe=cp.spawnSync('ffprobe',probeArgs,{encoding:'utf8',windowsHide:true});if(probe.status!==0)throw Error(probe.stderr);
 const info=JSON.parse(probe.stdout),stream=info.streams[0];
 if(stream.color_transfer!=='smpte2084'||stream.color_primaries!=='bt2020'||stream.color_space!=='bt2020nc'||stream.pix_fmt!=='yuv420p10le')throw Error('Unverified source encoding');
 if(!Number.isInteger(Number(stream.nb_frames))||index>=Number(stream.nb_frames))throw Error('Frame count/index unavailable or invalid');
 fs.writeFileSync(path.join(out,side+'.ffprobe.json'),probe.stdout);
 const output=path.join(out,side+'.png');
 const args=['-hide_banner','-nostdin','-n','-i',input,'-map','0:v:0','-vf',`select=eq(n\\,${index})`,'-fps_mode','passthrough','-frames:v','1','-pix_fmt','rgb48be',output];
 const run=cp.spawnSync('ffmpeg',args,{encoding:'utf8',windowsHide:true});fs.writeFileSync(path.join(out,side+'.decode.log'),run.stderr||'');
 if(run.error)throw run.error;if(run.status!==0||!fs.existsSync(output))throw Error('Decode failed');
 records.push({side,input:path.resolve(input),source_sha256:fingerprint(input),png_sha256:fingerprint(output),command:{executable:'ffmpeg',args},probe_command:{executable:'ffprobe',args:probeArgs}});
}
const version=cp.spawnSync('ffmpeg',['-version'],{encoding:'utf8',windowsHide:true});
fs.writeFileSync(path.join(out,'decode.json'),JSON.stringify({scope:'local diagnostic, original author PNG precision unresolved',zero_based_frame:index,pixel_format:'rgb48be',sampling_phase:'local explicit index',ffmpeg_version:version.stdout,records},null,2)+'\n');
console.log(`Decoded zero-based frame ${index} for both videos`);
