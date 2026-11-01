// Fresh instrumented copy only; never changes the clean official tree.
const fs=require('node:fs'), path=require('node:path'), crypto=require('node:crypto');
const [source,preprocessing,stockScore,output]=process.argv.slice(2);
if(!output||fs.existsSync(output))throw Error('Usage: clean-root preprocessing-root stock-score.json fresh-root');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const expected='0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a';
if(sha(fs.readFileSync(path.join(source,'hdrvdp3.m')))!==expected)throw Error('Clean reference hash differs');
fs.mkdirSync(output,{recursive:true});const copy=path.resolve(output,'instrumented');
fs.cpSync(source,copy,{recursive:true});
function patch(file,anchor,addition){const p=path.join(copy,file),s=fs.readFileSync(p,'utf8');
 if(s.split(anchor).length!==2)throw Error(`Nonunique anchor ${file}: ${anchor}`);
 fs.writeFileSync(p,s.replace(anchor,anchor+'\n'+addition));}
patch('hdrvdp3.m','M_itu2native = hdrvdp_iturgb2native( \'rgb-bt.2020\', IMG_E );',"iqm_capture_single('matrix',M_itu2native);");
patch('hdrvdp3.m','reference = fix_out_of_gamut( hdrvdp_colorspace_transform( reference, M_itu2native ), metric_par );',
 "iqm_capture_single('reference_native',reference);iqm_capture_single('test_native',test);");
for(const [anchor,tag,value] of [
 ['rho2 = create_cycdeg_image( img_sz*2, metric_par.pix_per_deg, metric_par.use_gpu );','rho','rho2'],
 ['mtf_filter = hdrvdp_mtf( rho2, metric_par );','mtf','mtf_filter'],
 ['R_LMSR = clamp( R_LMSR, 1e-8, 1e10 );','lmsr','R_LMSR'],
 ['L_adapt = hdrvdp_local_adapt( R_LMSR(:,:,1) + R_LMSR(:,:,2), metric_par.pix_per_deg );','adaptation','L_adapt'],
 ['P = P_C + P_R;','response','P'],
 ['BB = bands.get_band( bands.band_count(), 1 );','base_pre','BB']
 ])patch('hdrvdp_visual_pathway.m',anchor,`iqm_capture_single([name '_${tag}'],${value});`);
patch('hdrvdp_visual_pathway.m',"R_LMSR = reshape( reshape( L_O, width*height, img_ch )*M_img_lmsr, height, width, 4 );",
 "iqm_capture_single([name '_optical'],L_O);iqm_capture_single([name '_spectral_matrix'],M_img_lmsr);");
patch('hdrvdp_visual_pathway.m','bands = bands.set_band( bands.band_count(), 1, BB - mean(BB(:)) );',
 "for iqm_b=1:bands.band_count(),iqm_capture_single([name '_band' num2str(iqm_b)],bands.get_band(iqm_b,1));end");
patch('hdrvdp3.m','res.Q_JOD = 10-jod_pars(1)*max(10-res.Q,0).^jod_pars(2);',
 "iqm_capture_single('Q_err',Q_err);iqm_capture_single('Q',res.Q);iqm_capture_single('Q_JOD',res.Q_JOD);");
fs.copyFileSync('tests/iqm_capture_single.m',path.join(copy,'iqm_capture_single.m'));
const patchedHash=sha(fs.readFileSync(path.join(copy,'hdrvdp3.m')));
let runner=fs.readFileSync('tests/score_live_hdr_reference_frame.m','utf8');
runner=runner.replace(expected,patchedHash);
runner=runner.replace("assert(~exist(out,'dir'));assert(mkdir(out));", "assert(~exist(out,'dir'));assert(mkdir(out));global IQM_SINGLE_CAPTURE_ROOT;IQM_SINGLE_CAPTURE_ROOT=out;");
const stock=JSON.parse(fs.readFileSync(stockScore,'utf8'));
runner+=`\nassert(strcmp(result.Q_hex,'${stock.Q_hex}')&&strcmp(result.Q_JOD_hex,'${stock.Q_JOD_hex}'),'Instrumentation changed stock scores');\n`;
fs.writeFileSync(path.join(output,'run_capture.m'),runner);
fs.writeFileSync(path.join(output,'provenance.json'),JSON.stringify({clean_source:path.resolve(source),clean_sha256:expected,instrumented_sha256:patchedHash,preprocessing:path.resolve(preprocessing),stock_score:path.resolve(stockScore)},null,2));
console.log(path.resolve(output));
