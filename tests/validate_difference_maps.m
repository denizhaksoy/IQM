1;
function image=read_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=single(reshape(v,dims(2),dims(1))');
endfunction
args=argv(); root=args{1}; pkg load image; addpath(genpath('C:/HDR-VDP-3_Ref'));
p=hdrvdp_parse_options('quality',{}); pupil=dlmread(fullfile(root,'pupil.txt'));p.age=pupil(4); p.use_gpu=false;
addpath(fileparts(mfilename('fullpath')));[band_count,ppd]=hdrvdp_validation_band_count(root);
power=10^p.mask_p; pf=10^p.psych_func_slope/power; total=0;
for b=0:band_count-1
  r=read_map(fullfile(root,sprintf('gpu_ref_band%d.txt',b)))/2^b;
  t=read_map(fullfile(root,sprintf('gpu_test_band%d.txt',b)))/2^b;
  if b==band_count-1, r=r-mean(r(:)); t=t-mean(t(:)); endif
  difference=t-r;
  if b==band_count-1
    rho=create_cycdeg_image(size(difference),ppd/2^(b+1)*4);
    spectrum=fft2(difference);
    f=fopen(fullfile(root,'base_native_spectrum.txt'),'w'); assert(f>=0);
    interleaved=zeros(size(spectrum,1),2*size(spectrum,2),'single');
    interleaved(:,1:2:end)=real(spectrum); interleaved(:,2:2:end)=imag(spectrum);
    fprintf(f,'%d %d\n',size(interleaved)); fprintf(f,'%.9g\n',interleaved'); fclose(f);
    spectrum(1,1)=0;
    magnitude=abs(spectrum);
    f=fopen(fullfile(root,'base_native_magnitudes.txt'),'w'); assert(f>=0);
    fprintf(f,'%d %d\n',size(magnitude)); fprintf(f,'%.9g\n',magnitude'); fclose(f);
    index=find(abs(spectrum(:))==max(abs(spectrum(:))),1);
    freq=rho(index);
    f=fopen(fullfile(root,'base_native_selection.txt'),'w'); assert(f>=0);
    fprintf(f,'%d %.17g\n',index,freq); fclose(f);
    if exist(fullfile(root,'base_gpu_selection.txt'),'file')
      gpu_index=dlmread(fullfile(root,'base_gpu_selection.txt'));
      assert(isscalar(gpu_index) && gpu_index==index,'Base first-maximum index mismatch');
    endif
    ranked=sort(unique(magnitude(:)),'descend');
    runner_up=single(0);if numel(ranked)>1, runner_up=ranked(2);endif
    f=fopen(fullfile(root,'base_native_ranking.txt'),'w');assert(f>=0);
    fprintf(f,'index maximum next_distinct tie_count\n%d %.17g %.17g %d\n',...
      index,max(magnitude(:)),runner_up,nnz(magnitude==max(magnitude(:))));fclose(f);
    logs=read_map(fullfile(root,sprintf('csf_log_band%d.txt',b)));
    mean_log=mean(logs(:)); luminance=10.^mean_log;
    grid=log10(p.csf_lums); query=clamp(log10(luminance),grid(1),grid(end));
    parameters=zeros(1,5,'single'); parameters(1)=query;
    for k=1:4, parameters(k+1)=interp1(grid,p.csf_params(:,k+1),query); endfor
    f=fopen(fullfile(root,'base_native_csf_parameters.txt'),'w'); assert(f>=0);
    fprintf(f,'%.9g\n',parameters); fclose(f);
    f=fopen(fullfile(root,'base_native_adaptation.txt'),'w'); assert(f>=0);
    fprintf(f,'%.9g %.9g\n',mean_log,luminance); fclose(f);
    noise=1./hdrvdp_ncsf(freq,luminance,p);
    metadata=dlmread(fullfile(root,'base_csf.txt'));
    fprintf('base freq actual=%.17g expected=%.17g L actual=%.17g expected=%.17g noise_delta=%.17g\n',metadata(1),freq,metadata(2),luminance,metadata(3)-noise);
    assert(metadata(1)==freq && metadata(2)==luminance,'Base frequency/luminance mismatch');
    assert(metadata(3)==noise,'Base double noise mismatch');
  else
    noise=1./read_map(fullfile(root,sprintf('csf_band%d.txt',b)));
  endif
  mask=read_map(fullfile(root,sprintf('mask_denominator_band%d.txt',b)));
  excitation=sign(difference).*abs(difference).^power;
  D=excitation./sqrt(noise.^(2*power)+mask.^2);
  raw_actual=read_map(fullfile(root,sprintf('quality_difference_band%d.txt',b)));
  failed=find(raw_actual!=D);
  if ~isempty(failed)
    sensitivity=read_map(fullfile(root,sprintf('csf_band%d.txt',b)));
    neural=noise.^(2*power); if isscalar(neural), neural=repmat(neural,size(D)); endif
    square=mask.^2; denominator=sqrt(neural+square);
    diagnostic=[double(difference(failed)),double(sensitivity(failed)),double(mask(failed)),...
      repmat(power,numel(failed),1),repmat(pf,numel(failed),1),double(excitation(failed)),...
      double(neural(failed)),double(square(failed)),double(denominator(failed)),double(D(failed))];
    dlmwrite(fullfile(root,sprintf('difference_failure_band%d.txt',b)),diagnostic,'delimiter',' ','precision','%.17g');
  endif
  total+=nnz(raw_actual!=D);
  fprintf('band%d raw D mismatches=%d\n',b,nnz(raw_actual!=D));
  expected=sign(D).*abs(D).^pf;
  actual=read_map(fullfile(root,sprintf('difference_band%d.txt',b)));
  n=nnz(actual!=expected); total+=n;
  fprintf('band%d D mismatches=%d max=%.17g\n',b,n,max(abs(double(actual(:))-double(expected(:)))));
endfor
assert(total==0,sprintf('Difference-map mismatches=%d',total));
