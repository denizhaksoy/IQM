% Trace an isolated copy; never modify the installed official reference.
1;
function source=insert_once(source,anchor,extra)
 assert(numel(strfind(source,anchor))==1,['Non-unique trace anchor: ' anchor]);
 source=strrep(source,anchor,[anchor char(10) extra]);
endfunction
function write_values(path,a)
 assert(isreal(a)&&all(isfinite(a(:))));
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),columns(a));
 fprintf(f,'%.17g\n',a.');fclose(f);
endfunction
args=argv();visual=numel(args)>=2&&strcmp(args{2},'--visual');assert(numel(args)==1||(visual&&(numel(args)==2||numel(args)==4)));pattern='asymmetric';if numel(args)==4,assert(strcmp(args{3},'--pattern'));pattern=args{4};endif;assert(any(strcmp(pattern,{'asymmetric','impulse','hdr_ramp'})));out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));
assert(hdrvdp_version()==3.07);pkg load image;pkg load statistics;
fftw('threads',3);fftw('planner','estimate');
original=fileread(fullfile(official,'hdrvdp3.m'));source=original;
source=strrep(source,'function res = hdrvdp3(','function res = iqm_general_trace(');
assert(!strcmp(source,original));
source=strrep(source,'metric_par.base_dir = pathstr;',...
 ['metric_par.base_dir = ''' official ''';']);
assert(!isempty(strfind(source,['metric_par.base_dir = ''' official ''';'])));
source=insert_once(source,'log_La = log10(clamp( L_mean_adapt, CSF.csf_la(1), CSF.csf_la(end) ));',...
 ['res.iqm_trace = struct(''L_reference'',L_adapt_reference,''L_test'',L_adapt_test,' ...
 '''L_mean'',L_mean_adapt,''log_La'',log_La,''P_reference'',P_ref,''P_test'',P_test);' char(10) ...
 'for iqb=1:B_T.band_count(), for iqo=1:B_T.orient_count(iqb),' char(10) ...
 'res.iqm_trace.bands{iqb,iqo}=struct(''reference'',B_R.get_band(iqb,iqo),''test'',B_T.get_band(iqb,iqo));' char(10) ...
 'end;end']);
source=insert_once(source,'CSF_b = interp1( CSF.csf_log_la, CSF.S(:,b), log_La_rs );',...
 'res.iqm_trace.log_resized{b}=log_La_rs;res.iqm_trace.csf{b}=CSF_b;');
source=insert_once(source,'N_nCSF = 1./hdrvdp_ncsf( bb_freq, L_mean, metric_par );',...
 ['res.iqm_trace.base=struct(''difference'',band_diff,''spectrum'',band_diff_f,' ...
 '''index'',ind,''frequency'',bb_freq,''L_mean'',L_mean,''N_nCSF'',N_nCSF);']);
source=insert_once(source,'D = ex_diff./sqrt( N_nCSF.^(2*p) + N_mask.^2 );',...
 ['res.iqm_trace.masking{b,o}=struct(''self'',self_mask,''neighbor'',mask_xn,' ...
 '''denominator'',N_mask,''excitation'',ex_diff,''neural'',N_nCSF,''difference'',D,' ...
 '''p'',p,''q'',q,''pf'',pf,''self_scale'',k_mask_self,''neighbor_scale'',k_mask_xn);']);
source=insert_once(source,'D_p = D .* diff_mask_b;',...
 ['res.iqm_trace.masking{b,o}.comparison=diff_mask_b;' ...
 'res.iqm_trace.masking{b,o}.quality_difference=D_p;']);
source=insert_once(source,'Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();',...
 'res.iqm_trace.masking{b,o}.accumulated_error=Q_err;');
if visual
 visual_original=fileread(fullfile(official,'hdrvdp_visual_pathway.m'));
 visual_source=strrep(visual_original,'function [bands, L_adapt, bb_padvalue, P] = hdrvdp_visual_pathway(',...
  'function [bands, L_adapt, bb_padvalue, P, iqm_stage] = iqm_visual_trace(');
 assert(!strcmp(visual_original,visual_source));
 visual_source=insert_once(visual_source,'L_O = zeros_like(img);','iqm_stage=struct();');
 visual_source=insert_once(visual_source,'%TODO - MTF reduces luminance values','iqm_stage.retinal=L_O;');
 visual_source=insert_once(visual_source,'R_LMSR = clamp( R_LMSR, 1e-8, 1e10 ); % To avoid negative numbers',...
  'iqm_stage.matrix=M_img_lmsr;iqm_stage.LMSR_before=R_LMSR;iqm_stage.pn=pn;');
 visual_source=insert_once(visual_source,'L_adapt = hdrvdp_local_adapt( R_LMSR(:,:,1) + R_LMSR(:,:,2), metric_par.pix_per_deg );',...
  'iqm_stage.adaptation_input=R_LMSR(:,:,1)+R_LMSR(:,:,2);iqm_stage.adaptation=L_adapt;');
 visual_source=insert_once(visual_source,'lum_reduction = d_age.^2 ./ d_ref.^2;',...
  'iqm_stage.pupil=[L_a,d_ref,d_age,lum_reduction];iqm_stage.area=area;');
 visual_source=insert_once(visual_source,'P_LMR = zeros_like( R_LMSR );','iqm_stage.LMSR_after=R_LMSR;');
 visual_source=insert_once(visual_source,'P = P_C + P_R;',...
  'iqm_stage.channels=P_LMR;iqm_stage.cones=P_C;iqm_stage.rods=P_R;iqm_stage.P=P;');
 f=fopen(fullfile(out,'iqm_visual_trace.m'),'w');assert(f>=0);fputs(f,visual_source);fclose(f);
 for label={'ref','test'}
  n=label{1};old=sprintf('[B_%s, L_adapt_%s, bb_padvalue, P_%s]',upper(n(1)),ifelse(strcmp(n,'ref'),'reference','test'),n);
  assert(numel(strfind(source,old))==1);source=strrep(source,old,[old(1:end-1) ', visual_' n ']']);
 endfor
 assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);source=strrep(source,'hdrvdp_visual_pathway(','iqm_visual_trace(');
 source=insert_once(source,'for iqb=1:B_T.band_count(), for iqo=1:B_T.orient_count(iqb),',...
  'res.iqm_trace.visual_reference=visual_ref;res.iqm_trace.visual_test=visual_test;');
endif
f=fopen(fullfile(out,'iqm_general_trace.m'),'w');assert(f>=0);fputs(f,source);fclose(f);
addpath(out,'-begin');records={};
for shape={[80,96],[96,80]}
 hw=shape{1};h=hw(1);w=hw(2);[x,y]=meshgrid(0:w-1,0:h-1);
 reference=single(10)+single(.01)*single(mod(3*x+7*y+x.*y,101)-50);
 if strcmp(pattern,'impulse'),reference(:)=single(10);reference(1,8)=single(10.5);reference(h,w-1)=single(9.5);reference(floor(h/2),floor(w/3))=single(10.25);
 elseif strcmp(pattern,'hdr_ramp'),reference=single(10.^(-2+5*(3*x/(w-1)+2*y/(h-1))/5));endif
 test=reference;ys=floor(h*.25)+1:floor(h*.65);xs=floor(w*.55)+1:floor(w*.85);
 test(ys,xs)=reference(ys,xs)*single(1.4)+single(.25);
 options={'use_gpu',false,'age',24,'quiet',true};
 fftw('swisdom','');baseline=hdrvdp3('quality',test,reference,'luminance',30,options);
 fftw('swisdom','');traced=iqm_general_trace('quality',test,reference,'luminance',30,options);
 assert(isequal(baseline.Q,traced.Q)&&isequal(baseline.Q_JOD,traced.Q_JOD));
 folder=fullfile(out,sprintf('h%d_w%d',h,w));assert(mkdir(folder));
 trace=traced.iqm_trace;save('-mat7-binary',fullfile(folder,'trace.mat'),'trace','baseline');
 write_values(fullfile(folder,'reference.txt'),reference);write_values(fullfile(folder,'test.txt'),test);
 names={'L_reference','L_test','L_mean','log_La','P_reference','P_test'};classes=struct();
 for k=1:numel(names),n=names{k};write_values(fullfile(folder,[n '.txt']),trace.(n));classes.(n)=class(trace.(n));endfor
 for b=1:rows(trace.bands)
  for o=1:columns(trace.bands)
   if isempty(trace.bands{b,o}),continue;endif
   band=trace.bands{b,o};
   write_values(fullfile(folder,sprintf('band%d_o%d_reference.txt',b,o)),band.reference);
   write_values(fullfile(folder,sprintf('band%d_o%d_test.txt',b,o)),band.test);
  endfor
  write_values(fullfile(folder,sprintf('band%d_log_resized.txt',b)),trace.log_resized{b});
  write_values(fullfile(folder,sprintf('band%d_csf.txt',b)),trace.csf{b});
  mask=trace.masking{b,1};fields=fieldnames(mask);
  for j=1:numel(fields),n=fields{j};write_values(fullfile(folder,sprintf('band%d_mask_%s.txt',b,n)),mask.(n));endfor
 endfor
 base=trace.base;assert(numel(base.difference)==120);
 write_values(fullfile(folder,'base_difference.txt'),base.difference);
 write_values(fullfile(folder,'base_fft_real.txt'),real(base.spectrum));
 write_values(fullfile(folder,'base_fft_imag.txt'),imag(base.spectrum));
 write_values(fullfile(folder,'base_selection.txt'),[double(base.index),double(base.frequency),double(base.L_mean),double(base.N_nCSF)]);
 write_values(fullfile(folder,'scores.txt'),[baseline.Q,baseline.Q_JOD]);
 records{end+1}=struct('shape',hw,'classes',classes,'base_class',class(base.difference),...
  'base_log_class',class(trace.log_resized{end}),'base_shape',size(base.difference),...
  'Q',baseline.Q,'Q_JOD',baseline.Q_JOD,'score_identity',true);
 fprintf('Pathway %dx%d baseline score identity PASS; residual %dx%d\n',h,w,size(base.difference));
endfor
metadata=struct('version',hdrvdp_version(),'scope','Native full-pathway traces; no GPU integration claim',...
 'pattern',pattern,...
 'official_source_sha256',hash('sha256',original),'trace_source_sha256',hash('sha256',source),...
 'ppd',30,'age',24,'threads',3,'planner',fftw('planner'),'records',{records},...
 'padding','Official visual pathway, exact 2H x 2W FFT; no power-of-two rounding',...
 'layout','Text real matrices row-major; trace.mat retains native classes and complex values',...
 'DC','Base spectrum captured after official DC zero; base difference exactly as used by official metric');
if visual,metadata.visual_source_sha256=hash('sha256',visual_original);metadata.visual_trace_sha256=hash('sha256',visual_source);endif
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(metadata));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fputs(f,'Two native full-pathway traces; stock/traced Q and Q_JOD identical.');fclose(f);
