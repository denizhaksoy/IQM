% Fresh native MTF/FFT planning fixture; no GPU parity claim by itself.
1;
function write_real(path,image)
  f=fopen(path,'w'); assert(f>=0); fprintf(f,'%d %d\n',size(image)); fprintf(f,'%.17g\n',image'); fclose(f);
endfunction
function write_complex(path,image,column_order)
  f=fopen(path,'w'); assert(f>=0); fprintf(f,'%d %d\n',rows(image),2*columns(image));
  if !column_order, image=image.'; endif
  v=image(:); values=[real(v)';imag(v)']; fprintf(f,'%.17g\n',values); fclose(f);
endfunction
args=argv(); output=args{1}; side=32; if numel(args)>1,side=str2double(args{2});endif
assert(side==32||side==64); assert(!exist(output,'dir')); assert(mkdir(output));
official='C:/HDR-VDP-3_Ref'; if numel(args)>2,official=args{3};endif
addpath(genpath(official)); assert(hdrvdp_version()==3.07); pkg load image;
ppd=30; metric_par.mtf='hdrvdp'; metric_par.use_gpu=false; metric_par.surround='none';
p=[0.061466549455263 0.99727370023777070];
metric_par.mtf_params_a=[p(2)*0.426 p(2)*0.574 (1-p(2))*p(1) (1-p(2))*(1-p(1))];
metric_par.mtf_params_b=[0.028 0.37 37 360];
rho=create_cycdeg_image([2*side,2*side],ppd,false); filter=hdrvdp_mtf(rho,metric_par);
[x,y]=meshgrid(0:side-1,0:side-1);
reference=single(10)+single(0.01)*single(mod(3*x+7*y+x.*y,101)-50);
pattern='asymmetric';if numel(args)>3,pattern=args{4};endif
assert(any(strcmp(pattern,{'asymmetric','impulse','hdr_ramp'})));
if strcmp(pattern,'impulse')
  reference(:)=single(10);reference(1,8)=single(10.5);reference(side,side-1)=single(9.5);reference(floor(side/2),floor(side/3))=single(10.25);
elseif strcmp(pattern,'hdr_ramp')
  reference=single(10.^(-3+7*(3*x+2*y)/(5*(side-1))));
endif
test=reference*single(1.05);
f=fopen(fullfile(output,'fft_axis_order.txt'),'w'); fputs(f,'column'); fclose(f);
f=fopen(fullfile(output,'single_threads.txt'),'w'); fprintf(f,'%d\n',fftw('threads')); fclose(f);
fftw('swisdom','');
for label={'ref','test'}
  name=label{1}; input=reference; if strcmp(name,'test'),input=test;endif
  padding=hdrvdp_determine_padding(input,metric_par);
  prepared=padarray(input,[side,side],padding,'post');
  forward=fft2(prepared); filtered=forward.*filter; inverse=ifft2(filtered);
  native=clamp(fast_conv_fft(input,filter,padding),1e-5,1e10);
  staged=clamp(real(inverse(1:side,1:side)),1e-5,1e10);
  assert(isa(forward,'single') && isa(filtered,'single') && isequal(native,staged));
  write_real(fullfile(output,[name '_luminance.txt']),input);
  write_complex(fullfile(output,[name '_mtf_stage0.txt']),complex(prepared),true);
  write_complex(fullfile(output,[name '_mtf_stage2.txt']),filtered,true);
  write_complex(fullfile(output,[name '_native_forward.txt']),forward,false);
  write_complex(fullfile(output,[name '_native_inverse.txt']),inverse,false);
  write_real(fullfile(output,[name '_retinal.txt']),native);
  adapt_input=(native*0.65293140835027819)+(native*0.34706859164972181);
  sigma=10^-0.781367*ppd; fft_side=2*side;
  axis=(mod(0.5+(0:fft_side-1)/fft_side,1)-0.5)*(2*pi);
  [xx,yy]=meshgrid(axis,axis); gaussian=exp(-0.5*(xx.^2+yy.^2)*sigma^2);
  la_prepared=padarray(log(max(adapt_input,1e-6)),[side,side],'replicate','post');
  la_forward=fft2(la_prepared);la_filtered=la_forward.*gaussian;la_inverse=ifft2(la_filtered);
  adaptation=hdrvdp_local_adapt(adapt_input,ppd);
  assert(isequal(adaptation,exp(real(la_inverse(1:side,1:side)))));
  write_real(fullfile(output,[name '_adapt_input.txt']),adapt_input);
  write_real(fullfile(output,[name '_adaptation.txt']),adaptation);
  write_complex(fullfile(output,[name '_la_stage0.txt']),complex(la_prepared),true);
  write_complex(fullfile(output,[name '_la_stage2.txt']),la_filtered,true);
  write_complex(fullfile(output,[name '_la_native_forward.txt']),la_forward,false);
  write_complex(fullfile(output,[name '_la_native_inverse.txt']),la_inverse,false);
endfor
write_real(fullfile(output,'mtf_filter_double.txt'),filter);
write_real(fullfile(output,'la_filter_double.txt'),gaussian);
f=fopen(fullfile(output,'native_single_wisdom.txt'),'w'); fputs(f,fftw('swisdom')); fclose(f);
info=struct('scope','Native MTF/FFT fixture only;not GPU validation','reference',hdrvdp_version(),'input_shape',[side,side],'fft_shape',[2*side,2*side],'input_precision','single','filter_precision',class(filter),'fft_precision',class(forward),'ppd',ppd,'padding','symmetric post','intermediate_layout','stage0/2 column-major interleaved;native spectra row-major interleaved','ifft_normalization',1/(4*side^2),'planner',fftw('planner'),'threads',fftw('threads'));
info.pattern=pattern;info.local_adaptation='Official L+M coefficients,replicate post padding,sigma=10^-0.781367*ppd,native log/FFT/exp';
f=fopen(fullfile(output,'metadata.json'),'w'); fputs(f,jsonencode(info)); fclose(f);
fprintf('Native FFT fixture:input=%dx%d fft=%dx%d precision=%s threads=%d planner=%s staged_MTF_mismatches=0\n',side,side,2*side,2*side,class(forward),fftw('threads'),fftw('planner'));
