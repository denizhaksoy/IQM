% Native-only Double optical/adaptation capture from original encoded inputs.
1;
args=argv();assert(numel(args)==2);out=args{1};inputs=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);fftw('threads',3);fftw('planner','estimate');
source=fileread(fullfile(official,'hdrvdp3.m'));source=strrep(source,'function res = hdrvdp3(','function res = iqm_double_native(');
anchor='metric_par.base_dir = pathstr;';assert(numel(strfind(source,anchor))==1);source=strrep(source,anchor,['metric_par.base_dir = ''' official ''';']);
assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);source=strrep(source,'hdrvdp_visual_pathway(','iqm_double_visual(');
anchor='Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();';assert(numel(strfind(source,anchor))==1);
source=strrep(source,anchor,[anchor char(10) 'global iqm_double_quality;iqm_double_quality{b}=struct(''log_adaptation'',log_La_rs,''csf'',CSF_b,''noise'',N_nCSF+zeros(size(D)),''self'',self_mask,''neighbor'',mask_xn,''mask_noise'',N_mask,''band_diff'',band_diff,''excitation'',ex_diff,''difference'',D,''psychometric'',sign_pow(D,pf),''comparison'',diff_mask_b,''masked'',D_p,''parameters'',[p q pf k_mask_self k_mask_xn k_mask_xo],''error'',minkowski_sum(D_p(:),.8),''accumulated'',Q_err,''grid'',CSF.csf_log_la,''csf_table'',CSF.S(:,b));if b==B_T.band_count(),iqm_double_quality{b}.base_metadata=[double(ind),bb_freq,L_mean];iqm_double_quality{b}.base_fft=band_diff_f;endif']);
visual=fileread(fullfile(official,'hdrvdp_visual_pathway.m'));visual=strrep(visual,' = hdrvdp_visual_pathway(',' = iqm_double_visual(');
visual=strrep(visual,'fast_conv_fft(','iqm_double_conv(');visual=strrep(visual,'hdrvdp_local_adapt(','iqm_double_adapt(');
anchor='R_LMSR = clamp( R_LMSR, 1e-8, 1e10 ); % To avoid negative numbers';assert(numel(strfind(visual,anchor))==1);
visual=strrep(visual,anchor,['global iqm_double_trace;iqm_double_trace{end+1}=struct(''input'',img,''optical'',L_O,''matrix'',M_img_lmsr,''response'',R_LMSR);' char(10) anchor]);
anchor='L_adapt = iqm_double_adapt( R_LMSR(:,:,1) + R_LMSR(:,:,2), metric_par.pix_per_deg );';assert(numel(strfind(visual,anchor))==1);
visual=strrep(visual,anchor,[anchor char(10) 'iqm_double_trace{end}.clamped=R_LMSR;iqm_double_trace{end}.adaptation_input=R_LMSR(:,:,1)+R_LMSR(:,:,2);iqm_double_trace{end}.adaptation=L_adapt;']);
anchor='P_LMR = zeros_like( R_LMSR );';assert(numel(strfind(visual,anchor))==1);
visual=strrep(visual,anchor,[anchor char(10) 'iqm_double_trace{end}.post_pupil=R_LMSR;iqm_double_trace{end}.pupil_reduction=lum_reduction;iqm_double_trace{end}.geomean=L_a;iqm_double_trace{end}.lut=[pn.Y{1}(:),pn.jnd{1}(:),pn.jnd{2}(:)];']);
anchor='P = P_C + P_R;';assert(numel(strfind(visual,anchor))==1);
visual=strrep(visual,anchor,[anchor char(10) 'iqm_double_trace{end}.jnd=P_LMR;iqm_double_trace{end}.P=P;']);
anchor='bands = bands.set_band( bands.band_count(), 1, BB - mean(BB(:)) );';assert(numel(strfind(visual,anchor))==1);
visual=strrep(visual,anchor,['iqm_double_trace{end}.base_mean=mean(BB(:));iqm_double_trace{end}.base_pre=BB;' char(10) anchor char(10) 'iqm_double_trace{end}.bands={};for capture_band=1:bands.band_count(),iqm_double_trace{end}.bands{capture_band}=bands.get_band(capture_band,1);endfor']);
conv=fileread(fullfile(official,'utils','fast_conv_fft.m'));
anchor="fX = fft2( padarray( X, pad_size, pad_value, 'post' ) );";assert(numel(strfind(conv,anchor))==1);
conv=strrep(conv,anchor,"padded=padarray(X,pad_size,pad_value,'post');fX=fft2(padded);");
anchor='Y = Yl(1:size(X,1),1:size(X,2));';assert(numel(strfind(conv,anchor))==1);
conv=strrep(conv,anchor,[anchor char(10) 'global iqm_double_convs;iqm_double_convs{end+1}=struct(''input'',X,''filter'',fH,''padding'',pad_value,''padded'',padded,''fft'',fX,''inverse'',Yl,''output'',Y);']);
gauss=fileread(fullfile(official,'utils','fast_gauss.m'));
% Preserve its local convolution, instrumented identically to the optical one.
start=strfind(gauss,'function Y = fast_conv_fft(');assert(numel(start)==1);gauss=[gauss(1:start-1) conv];
gauss=strrep(gauss,'function Y = fast_gauss(','function Y = iqm_double_gauss(');
adapt=fileread(fullfile(official,'utils','hdrvdp_local_adapt.m'));adapt=strrep(adapt,'function L_la = hdrvdp_local_adapt(','function L_la = iqm_double_adapt(');adapt=strrep(adapt,'fast_gauss(','iqm_double_gauss(');
optical_conv=strrep(conv,'function Y = fast_conv_fft(','function Y = iqm_double_conv(');
for item={{'iqm_double_native.m',source},{'iqm_double_visual.m',visual},{'iqm_double_conv.m',optical_conv},{'iqm_double_gauss.m',gauss},{'iqm_double_adapt.m',adapt}}
 f=fopen(fullfile(out,item{1}{1}),'w');fputs(f,item{1}{2});fclose(f);
endfor
addpath(out,'-begin');global iqm_double_trace iqm_double_convs iqm_double_quality;records={};
for display={'led-lcd-srgb','led-lcd-wcg','oled'}
 name=display{1};images={};for side=1:2,f=fopen(fullfile(inputs,[name '_' num2str(side) '_input.f32']),'rb','ieee-le');assert(f>=0);a=fread(f,Inf,'single=>single');fclose(f);assert(numel(a)==27648);images{side}=reshape(a,[96,96,3]);endfor
 options={'use_gpu',false,'quiet',true,'age',24,'rgb_display',name};
 fftw('dwisdom','');log=evalc("stock=hdrvdp3('quality',images{2},images{1},'srgb-display',30,options);");
 iqm_double_trace={};iqm_double_convs={};iqm_double_quality={};fftw('dwisdom','');log=evalc("traced=iqm_double_native('quality',images{2},images{1},'srgb-display',30,options);");
 assert(isequal(typecast(stock.Q,'uint64'),typecast(traced.Q,'uint64'))&&isequal(typecast(stock.Q_JOD,'uint64'),typecast(traced.Q_JOD,'uint64')));
 assert(numel(iqm_double_trace)==2&&numel(iqm_double_convs)==8);
 f=fopen(fullfile(out,[name '_fftw.wisdom']),'w');fputs(f,fftw('dwisdom'));fclose(f);
 assert(numel(iqm_double_quality)==5);for b=1:5,t=iqm_double_quality{b};for field=fieldnames(t)',v=t.(field{1});assert(isa(v,'double'));f=fopen(fullfile(out,[name '_quality_band' num2str(b) '_' field{1} '.f64']),'wb','ieee-le');if strcmp(field{1},'base_fft'),fwrite(f,[real(v(:))';imag(v(:))'],'double');else,fwrite(f,v,'double');endif;fclose(f);endfor;endfor
 f=fopen(fullfile(out,[name '_quality.f64']),'wb','ieee-le');fwrite(f,[iqm_double_quality{5}.accumulated,stock.Q,stock.Q_JOD],'double');fclose(f);
 for side=1:2
  t=iqm_double_trace{side};prefix=fullfile(out,[name '_' num2str(side)]);
  for field={'input','optical','matrix','response','clamped','adaptation_input','adaptation','post_pupil','pupil_reduction','geomean','lut','jnd','P','base_mean','base_pre'}
   v=t.(field{1});assert(isa(v,'double'));f=fopen([prefix '_' field{1} '.f64'],'wb','ieee-le');fwrite(f,v,'double');fclose(f);
  endfor
  for b=1:numel(t.bands),v=t.bands{b};assert(isa(v,'double'));f=fopen([prefix '_band' num2str(b) '.f64'],'wb','ieee-le');fwrite(f,v,'double');fclose(f);endfor
  for k=1:4
   c=iqm_double_convs{(side-1)*4+k};assert(strcmp(c.padding,{'symmetric','symmetric','symmetric','replicate'}{k}));
   for field={'input','filter','padded','fft','inverse','output'}
    v=c.(field{1});assert(isa(v,'double'));f=fopen([prefix '_conv' num2str(k) '_' field{1} '.f64'],'wb','ieee-le');if strcmp(field{1},'fft'),fwrite(f,[real(v(:))';imag(v(:))'],'double');else,fwrite(f,v,'double');endif;fclose(f);
   endfor
  endfor
 endfor
 records{end+1}=struct('display',name,'Q',stock.Q,'Q_JOD',stock.Q_JOD);fprintf('%s optical/adaptation Double captures;stock/traced score bits exact\n',name);
endfor
[lo0,hi0,lo,b]=sp0Filters();f=fopen(fullfile(out,'filters.f64'),'wb','ieee-le');for v={lo0,hi0,lo,b},fwrite(f,v{1},'double');endfor;fclose(f);
f=fopen(fullfile(out,'pyramid_backend.txt'),'w');fputs(f,which('corrDn'));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native Double stage captures only;no independent optical implementation acceptance','input_source',inputs,'precision','original Single encoded;Double pathway/FFTW','parameters','96x96;30ppd;age24;defaultsp0;FFTW3threads3/estimate/dwisdom cleared','layout','column-major planar;FFT complex interleaved real/imag;192x192 post padding;native DC unchanged','convolution_order','reference RGB optical symmetric then log-adaptation replicate;then test same','records',{records})));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Native stock/traced score bits equal;3spectra/2sides/4Doubleconvolutions captured. Native characterization only.');fclose(f);


