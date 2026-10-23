1;
function image=read_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=single(reshape(v,dims(2),dims(1))');
endfunction
function image=read_double_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=reshape(v,dims(2),dims(1))';
endfunction
args=argv(); root=args{1}; pkg load image; pkg load statistics;
addpath(genpath('C:/HDR-VDP-3_Ref'));
reference=read_map(fullfile(root,'ref_luminance.txt'));
test=read_map(fullfile(root,'test_luminance.txt'));
mask=abs(test-reference)./reference>0.001;
addpath(fileparts(mfilename('fullpath')));[band_count,ppd]=hdrvdp_validation_band_count(root);
f=fopen(fullfile(root,'quality.txt')); assert(f>=0); values=fscanf(f,'%f'); fclose(f); assert(numel(values)==band_count+3);
error=0; total=0;
for b=0:band_count-1
  D=read_map(fullfile(root,sprintf('quality_difference_band%d.txt',b)));
  resized_mask=imresize(double(mask),size(D));
  exported=read_double_map(fullfile(root,sprintf('quality_mask_band%d.txt',b)));
  assert(isequal(size(exported),size(resized_mask)) && all(exported(:)==resized_mask(:)),'Double comparison mask mismatch');
  f=fopen(fullfile(root,sprintf('quality_mask_native_band%d.txt',b)),'w'); assert(f>=0);
  fprintf(f,'%d %d\n',size(resized_mask)); fprintf(f,'%.17g\n',resized_mask'); fclose(f);
  Dp=D.*resized_mask;
  band=sum(abs(Dp(:)).^0.8/numel(Dp)).^(1/0.8);
  n=values(b+1)!=band; total+=n;
  fprintf('band%d quality actual=%.17g expected=%.17g mismatch=%d\n',b,values(b+1),band,n);
  error=error+band/band_count;
endfor
Q=6-log(exp(-4)+error); Q_JOD=10-0.5200*max(10-Q,0).^1.2812;
expected=[error,Q,Q_JOD];
fprintf('staged scores actual: %.17g %.17g %.17g\n',values(band_count+1:band_count+3));
fprintf('staged scores expect: %.17g %.17g %.17g class=%s\n',expected,class(expected));
total+=nnz(values(band_count+1:band_count+3)'!=expected);
assert(total==0,sprintf('Quality pooling mismatches=%d',total));
if any(strcmp(args,'--official'))
  if any(strcmp(args,'--fresh-fft-plans'))
    fftw('threads',3);fftw('swisdom','');
    fprintf('Official quality FFTW threads=%d planner=%s;single wisdom reset\n',fftw('threads'),fftw('planner'));
  endif
  result=hdrvdp3('quality',test,reference,'luminance',ppd,{'use_gpu',false,'age',dlmread(fullfile(root,'pupil.txt'))(4),'quiet',true});
  fprintf('OFFICIAL Q actual=%.17g expected=%.17g Q_JOD actual=%.17g expected=%.17g\n',values(band_count+2),result.Q,values(band_count+3),result.Q_JOD);
  save(fullfile(root,'official_quality.mat'),'result');
  assert(values(band_count+2)==result.Q && values(band_count+3)==result.Q_JOD,'Official end-to-end quality mismatch');
endif
