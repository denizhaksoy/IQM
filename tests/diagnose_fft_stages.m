1;
function img=load_real(path)
  fid=fopen(path,'r'); assert(fid>=0); d=fscanf(fid,'%d',2); v=fscanf(fid,'%f'); fclose(fid);
  assert(numel(v)==prod(d)); img=single(reshape(v,d(2),d(1))');
end
function z=load_complex(path)
  interleaved=load_real(path); z=complex(interleaved(:,1:2:end),interleaved(:,2:2:end));
end
function report(label,a,b)
  delta=double(a)-double(b); assert(all(isfinite(delta(:))));
  fprintf('%s max=%.17g RMSE=%.17g mismatches=%d\n',label,max(abs(delta(:))),sqrt(mean(abs(delta(:)).^2)),nnz(delta));
end
args=argv(); root=args{1}; pkg load image;
strict_all=numel(args)>1 && strcmp(args{2},'--strict-all-stages');
strict_transforms=strict_all || (numel(args)>1 && strcmp(args{2},'--strict-transforms'));
column_first=false;
if exist(fullfile(root,'fft_axis_order.txt'),'file')
  column_first=strcmp(strtrim(fileread(fullfile(root,'fft_axis_order.txt'))),'column');
end
addpath(genpath('C:/HDR-VDP-3_Ref'));
par=[0.061466549455263 0.99727370023777070];
metric_par.mtf='hdrvdp';
metric_par.mtf_params_a=[par(2)*0.426 par(2)*0.574 (1-par(2))*par(1) (1-par(2))*(1-par(1))];
metric_par.mtf_params_b=[0.028 0.37 37 360];
for label={'ref','test'}
  name=label{1};
  for mode={'mtf','la'}
    kind=mode{1}; prefix=fullfile(root,[name '_' kind '_']);
    if strcmp(kind,'mtf')
      img=load_real(fullfile(root,[name '_luminance.txt']));
      prepared=padarray(img,[64 64],'symmetric','post');
      filter=single(hdrvdp_mtf(create_cycdeg_image([128 128],30,false),metric_par));
    else
      img=load_real(fullfile(root,[name '_adapt_input.txt']));
      prepared=padarray(log(max(img,1e-6)),[64 64],'replicate','post');
      f=(mod(0.5+(0:127)/128,1)-0.5)*2*pi;
      [xx,yy]=meshgrid(f,f); sigma=10^-0.781367*30;
      filter=single(exp(-0.5*(xx.^2+yy.^2)*sigma^2));
    end
    gpu_filter=load_real([prefix 'filter.txt']);
    gpu0=load_complex([prefix 'stage0.txt']); gpu1=load_complex([prefix 'stage1.txt']);
    gpu2=load_complex([prefix 'stage2.txt']); gpu3=load_complex([prefix 'stage3.txt']);
    if column_first
      gpu0=gpu0.'; gpu1=gpu1.'; gpu2=gpu2.';
      storage=fullfile(root,'fft_inverse_storage.txt');
      if ~exist(storage,'file') || strcmp(strtrim(fileread(storage)),'column'), gpu3=gpu3.'; end
    end
    tag=[name ' ' kind ' '];
    report([tag 'filter'],gpu_filter,filter);
    if strict_all, assert(isequal(gpu_filter,filter),'Filter mismatch'); end
    report([tag 'prepare'],gpu0,prepared);
    if strict_all, assert(isequal(gpu0,prepared),'Prepare mismatch'); end
    % Each FFT is compared on exactly the same input to localize divergence.
    % Official forward input is real, not a complex array with zero imaginary data.
    expected1=fft2(real(gpu0));
    report([tag 'forward_same_input'],gpu1,expected1);
    if strict_transforms, assert(isequal(gpu1,expected1),'Forward FFT mismatch on identical input'); end
    report([tag 'forward_complex_input_diagnostic'],gpu1,fft2(gpu0));
    report([tag 'multiply_same_input'],gpu2,gpu1.*gpu_filter);
    if strict_all, assert(isequal(gpu2,gpu1.*gpu_filter),'Spectrum multiplication mismatch'); end
    expected3=ifft2(gpu2);
    report([tag 'inverse_same_input'],gpu3,expected3);
    if strict_transforms, assert(isequal(gpu3,expected3),'Inverse FFT mismatch on identical input'); end
    expected=real(gpu3(1:64,1:64));
    if strcmp(kind,'mtf'), expected=clamp(expected,1e-5,1e10); output='retinal';
    else, expected=exp(expected); output='adaptation'; end
    actual=load_real(fullfile(root,[name '_' output '.txt']));
    report([tag 'finish_same_input'],actual,expected);
    if strict_all, assert(isequal(actual,expected),'Finish mismatch'); end
  end
end
if strict_transforms, fprintf('Integrated same-input forward/inverse FFT: PASS\n'); end
if strict_all, fprintf('All integrated filter stages: PASS\n'); end
