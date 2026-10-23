% Official v3.0.7 rectangular MTF/LA staged references;fresh exports only.
1;
function write_real(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',size(a));fprintf(f,'%.17g\n',a');fclose(f);
endfunction
function write_complex(path,a,column_order)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),2*columns(a));
 if !column_order,a=a.';endif
 v=a(:);fprintf(f,'%.17g\n',[real(v)';imag(v)']);fclose(f);
endfunction
args=argv();out=args{1};h=str2double(args{2});w=str2double(args{3});pattern=args{4};
assert((h==64&&w==128)||(h==128&&w==64));
assert(any(strcmp(pattern,{'asymmetric','impulse','hdr_ramp'})));
assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;
fftw('threads',3);fftw('swisdom','');ppd=30;
p=hdrvdp_parse_options('quality',{});p.use_gpu=false;p.surround='none';
filter=hdrvdp_mtf(create_cycdeg_image([2*h,2*w],ppd,false),p);
[x,y]=meshgrid(0:w-1,0:h-1);
reference=single(10)+single(.01)*single(mod(3*x+7*y+x.*y,101)-50);
if strcmp(pattern,'impulse')
 reference(:)=single(10);reference(1,8)=single(10.5);reference(h,w-1)=single(9.5);reference(floor(h/2),floor(w/3))=single(10.25);
elseif strcmp(pattern,'hdr_ramp')
 reference=single(10.^(-3+7*(3*x/(w-1)+2*y/(h-1))/5));
endif
sigma=10^-0.781367*ppd;
ax=(mod(.5+(0:2*w-1)/(2*w),1)-.5)*(2*pi);
ay=(mod(.5+(0:2*h-1)/(2*h),1)-.5)*(2*pi);
[xx,yy]=meshgrid(ax,ay);gaussian=exp(-.5*(xx.^2+yy.^2)*sigma^2);
for label={'ref','test'}
 name=label{1};input=reference;if strcmp(name,'test'),input=input*single(1.05);endif
 padding=hdrvdp_determine_padding(input,p);assert(strcmp(padding,'symmetric'));
 prepared=padarray(input,[h,w],padding,'post');forward=fft2(prepared);filtered=forward.*filter;inverse=ifft2(filtered);
 retinal=clamp(fast_conv_fft(input,filter,padding),1e-5,1e10);
 assert(isequal(retinal,clamp(real(inverse(1:h,1:w)),1e-5,1e10)));
 write_real(fullfile(out,[name '_luminance.txt']),input);
 write_complex(fullfile(out,[name '_mtf_stage0.txt']),complex(prepared),true);
 write_complex(fullfile(out,[name '_mtf_stage2.txt']),filtered,true);
 write_complex(fullfile(out,[name '_native_forward.txt']),forward,false);
 write_complex(fullfile(out,[name '_native_inverse.txt']),inverse,false);
 write_real(fullfile(out,[name '_retinal.txt']),retinal);
 adapt=(retinal*0.65293140835027819)+(retinal*0.34706859164972181);
 prepared=padarray(log(max(adapt,1e-6)),[h,w],'replicate','post');
 forward=fft2(prepared);filtered=forward.*gaussian;inverse=ifft2(filtered);
 adaptation=hdrvdp_local_adapt(adapt,ppd);
 assert(isequal(adaptation,exp(real(inverse(1:h,1:w)))));
 write_real(fullfile(out,[name '_adapt_input.txt']),adapt);
 write_real(fullfile(out,[name '_adaptation.txt']),adaptation);
 write_complex(fullfile(out,[name '_la_stage0.txt']),complex(prepared),true);
 write_complex(fullfile(out,[name '_la_stage2.txt']),filtered,true);
 write_complex(fullfile(out,[name '_la_native_forward.txt']),forward,false);
 write_complex(fullfile(out,[name '_la_native_inverse.txt']),inverse,false);
endfor
write_real(fullfile(out,'mtf_filter_double.txt'),filter);write_real(fullfile(out,'la_filter_double.txt'),gaussian);
f=fopen(fullfile(out,'native_single_wisdom.txt'),'w');assert(f>=0);fputs(f,fftw('swisdom'));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('scope','Native staged rectangular MTF/LA only;not full quality',...
 'version',hdrvdp_version(),'reference',which('hdrvdp_local_adapt'),'pattern',pattern,...
 'input_shape',[h,w],'fft_shape',[2*h,2*w],'input_precision','single','filter_precision','double',...
 'ppd',ppd,'single_threads_requested',3,'planner',fftw('planner'),...
 'padding','MTF symmetric post;LA replicate post','normalization',1/(4*h*w),...
 'stage_layout','stage0/2 column-major interleaved;forward/inverse row-major interleaved',...
 'DC_processing','none','adaptation_input','official L+M from native MTF output')));fclose(f);
fprintf('Native rectangular MTF/LA %dx%d %s staged comparisons exact\n',h,w,pattern);
