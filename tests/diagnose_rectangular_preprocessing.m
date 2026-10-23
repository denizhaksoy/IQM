% Fresh-process native FFT comparison, without preceding JND diagnostic FFTs.
1;
function a=read_map(path)
 f=fopen(path,'r');assert(f>=0);d=fscanf(f,'%d',2);v=fscanf(f,'%f');fclose(f);
 a=single(reshape(v,d(2),d(1)).');
endfunction
args=argv();root=args{1};
addpath(genpath('C:/HDR-VDP-3_Ref'));pkg load image;
fftw('threads',3);fftw('swisdom','');
p=hdrvdp_parse_options('quality',{});p.use_gpu=false;p.surround='none';
for label={'ref','test'}
 name=label{1};a=read_map(fullfile(root,[name '_luminance.txt']));
 filter=hdrvdp_mtf(create_cycdeg_image(size(a)*2,30,false),p);
 native=clamp(fast_conv_fft(a,filter,hdrvdp_determine_padding(a,p)),1e-5,1e10);
 gpu=read_map(fullfile(root,[name '_retinal.txt']));delta=double(gpu)-double(native);
 fprintf('%s fresh MTF mismatches=%d max=%.17g\n',name,nnz(delta),max(abs(delta(:))));
endfor
