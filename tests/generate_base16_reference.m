% Native single16x16 residual spectra, magnitudes and column-major first maxima.
1;args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
fftw('threads',3);fftw('swisdom','');rng(162026);
spyr_mode=numel(args)>1&&strcmp(args{2},'spyr');
if spyr_mode,addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);endif
fi=fopen(fullfile(out,'input.txt'),'w');fo=fopen(fullfile(out,'expected.txt'),'w');
assert(fi>=0&&fo>=0);fprintf(fi,'64 512\n');fprintf(fo,'64 769\n');
[x,y]=meshgrid(0:15,0:15);indices=[];
for k=0:63
 a=single(randn(16,16)*2^(-10+mod(k,21)));
 if k==0,a(:)=0;elseif k==1,a(:)=1;
 elseif k==2,a(:)=0;a(3,12)=1;
 elseif k==3,a=single(cos(2*pi*x/16));
 elseif k==4,a=single(cos(2*pi*y/16));
 elseif k==5,a=single(cos(2*pi*x/16)+cos(4*pi*y/16));
 elseif k==6,a=single((-1).^(x+y));
 elseif k==7,a=single(cos(2*pi*x/16)+cos(4*pi*y/16));a(3,12)+=eps('single');endif
 if spyr_mode
   image=single(randn(128,128)*2^(-10+mod(k,21)));
   if k==0,image(:)=0;elseif k==1,image(:)=1;
   elseif k==2,image(:)=0;image(3,91)=1;endif
   bands=hdrvdp_spyr('sp0Filters');bands=bands.decompose(image,30);
   assert(bands.band_count()==5);a=bands.get_band(5,1);assert(isequal(size(a),[16,16]));a=a-mean(a(:));
 endif
 spectrum=fft2(a);assert(isa(spectrum,'single'));magnitudes=abs(spectrum);magnitudes(1,1)=0;
 index=find(magnitudes(:)==max(magnitudes(:)),1)-1;indices(end+1)=index;
 values=reshape(a.',[],1);fprintf(fi,'%.17g\n',[values';zeros(1,256,'single')]);
 row=reshape(spectrum.',[],1);fprintf(fo,'%.17g\n',[real(row)';imag(row)']);
 fprintf(fo,'%.17g\n',magnitudes');fprintf(fo,'%d\n',index);
endfor
fclose(fi);fclose(fo);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native residual16 only','official_spyr_inputs',spyr_mode,'precision','single','threads',fftw('threads'),'planner',fftw('planner'),'input_layout','64 matrices row-major interleaved complex, imaginary0','output_layout','per matrix512 FFT components,256 DC-zero magnitudes,zero-based column-major first-max index','normalization',1,'DC_processing','SPYR mode subtracts native mean beforeexport;GPU only zeros DC magnitude and preserves raw spectrum','indices',indices)));fclose(f);
f=fopen(fullfile(out,'native_single_wisdom.txt'),'w');fputs(f,fftw('swisdom'));fclose(f);
fprintf('Native residual16:64 matrices,32768 spectrum floats,16384 magnitudes,64 indices\n');
