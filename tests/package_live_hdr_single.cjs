// Local reproducibility bundle; does not publish or modify the dataset source.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [exe,data,frame0,frame30,stock0,stock30,out,lowerPng,lowerStock]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: exe data-root frame0-png-root frame30-png-root stock0.json stock30.json fresh-root');
fs.mkdirSync(out,{recursive:true});const entries=[];
function copy(source,relative){const target=path.join(out,relative);fs.mkdirSync(path.dirname(target),{recursive:true});fs.copyFileSync(source,target);entries.push({file:relative,sha256:crypto.createHash('sha256').update(fs.readFileSync(target)).digest('hex')});}
copy(exe,'live-frame-single.exe');copy('tests/run_live_hdr_frame.cjs','run_live_hdr_frame.cjs');
for(const name of ['ciexyz31.csv','emission_spectra_oled.csv','log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt'])copy(path.join(data,name),'data/'+name);
for(const [label,root,score] of [['frame0000',frame0,stock0],['frame0030',frame30,stock30]]){
 for(const name of ['reference.png','test.png'])copy(path.join(root,name),label+'/'+name);copy(score,label+'/stock_score.json');
}
if(Boolean(lowerPng)!==Boolean(lowerStock))throw Error('Both low-resolution arguments are required');
if(lowerPng){for(const name of ['reference.png','test.png'])copy(path.join(lowerPng,name),'lowres540/'+name);copy(lowerStock,'lowres540/stock_score.json');}
copy('validation/live_hdr_639290787607997134/metadata/README.txt','DATASET_README.txt');
copy('tests/live_hdr_protocol.json','protocol.json');
const sources=['tests/single_fftw_backend.h','lib/stb/stb_image.h'];
for(const name of fs.readdirSync('tests/live_hdr_preprocessing'))sources.push('tests/live_hdr_preprocessing/'+name);
for(const name of ['pq_single','linear_color_single','optical_single','optical_response_single','jnd','jnd_single','sp0_single','sp0_model_double','general_quality_single','quality','csf','resize','spectral','spectral_loader','mtf']){
 const p='src/hdrvdp3/'+name+'.cpp';if(fs.existsSync(p))sources.push(p);
}
for(const p of sources)copy(p,'source/'+p);
for(const name of fs.readdirSync('include/IQM/hdrvdp3'))if(name.endsWith('.h'))copy('include/IQM/hdrvdp3/'+name,'source/include/IQM/hdrvdp3/'+name);
fs.writeFileSync(path.join(out,'run_demo.ps1'),`param(
 [ValidateSet('frame0000','frame0030','lowres540')][string] $Frame = 'frame0000',
 [string] $FFTW = 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll'
)
$ErrorActionPreference = 'Stop'
$demoOutput = Join-Path $PSScriptRoot ('results_' + $Frame + '_' + [DateTime]::UtcNow.Ticks)
node (Join-Path $PSScriptRoot 'run_live_hdr_frame.cjs') (Join-Path $PSScriptRoot 'live-frame-single.exe') (Join-Path $PSScriptRoot ($Frame + '/reference.png')) (Join-Path $PSScriptRoot ($Frame + '/test.png')) (Join-Path $PSScriptRoot 'data') $FFTW (Join-Path $PSScriptRoot ($Frame + '/stock_score.json')) $demoOutput
if ($LASTEXITCODE -ne 0) { throw 'Exact comparison failed; inspect the output directory' }
Write-Output $demoOutput
`);
entries.push({file:'run_demo.ps1',sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(out,'run_demo.ps1'))).digest('hex')});
fs.writeFileSync(path.join(out,'run_demo.cjs'),`const path=require('node:path'),cp=require('node:child_process'),fs=require('node:fs'),crypto=require('node:crypto');
const frame=process.argv[2]||'frame0000';if(!['frame0000','frame0030','lowres540'].includes(frame))throw Error('Choose frame0000, frame0030 or lowres540');
const dll=process.argv[3]||'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll';
const manifest=JSON.parse(fs.readFileSync(path.join(__dirname,'files.json'),'utf8'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
for(const entry of manifest.entries)if(hash(path.join(__dirname,entry.file))!==entry.sha256)throw Error('Bundle hash mismatch: '+entry.file);
if(hash(dll)!==manifest.external_dependency.sha256)throw Error('FFTW dependency differs');
const output=path.join(__dirname,'results_'+frame+'_'+Date.now());
const args=[path.join(__dirname,'run_live_hdr_frame.cjs'),path.join(__dirname,'live-frame-single.exe'),path.join(__dirname,frame,'reference.png'),path.join(__dirname,frame,'test.png'),path.join(__dirname,'data'),dll,path.join(__dirname,frame,'stock_score.json'),output];
const run=cp.spawnSync(process.execPath,args,{stdio:'inherit',windowsHide:true});if(run.error)throw run.error;console.log(output);process.exitCode=run.status===0?0:2;
`);
entries.push({file:'run_demo.cjs',sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(out,'run_demo.cjs'))).digest('hex')});
fs.writeFileSync(path.join(out,'files.json'),JSON.stringify({scope:'local two-frame CPU Single diagnostic, not full LIVE dataset reproduction',external_dependency:{name:'Octave libfftw3f-3.dll',sha256:'2dda2a429ad7a9088260c21e0e2e5fd13ff66771832f82d4da3e836b2875b55e'},entries},null,2)+'\n');
console.log(path.resolve(out));
