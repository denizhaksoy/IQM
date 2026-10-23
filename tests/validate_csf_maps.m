1;
function image=read_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=single(reshape(v,dims(2),dims(1))');
endfunction
args=argv(); root=args{1}; pkg load image; addpath(genpath('C:/HDR-VDP-3_Ref'));
p=hdrvdp_parse_options('quality',{}); pupil=dlmread(fullfile(root,'pupil.txt'));p.age=pupil(4); p.use_gpu=false;
addpath(fileparts(mfilename('fullpath')));[band_count,ppd]=hdrvdp_validation_band_count(root);
la=logspace(-5,5,256); grid=log10(la);
mean_adapt=(read_map(fullfile(root,'ref_adaptation.txt'))+read_map(fullfile(root,'test_adaptation.txt')))./2;
logs=log10(clamp(mean_adapt,la(1),la(end)));
actual=read_map(fullfile(root,'csf_log_adaptation.txt'));
fprintf('log adaptation mismatches=%d max=%.17g\n',nnz(actual!=logs),max(abs(double(actual(:))-double(logs(:)))));
total=nnz(actual!=logs);
for b=0:band_count-1
  actual=read_map(fullfile(root,sprintf('csf_band%d.txt',b)));
  resized=clamp(imresize(logs,size(actual)),grid(1),grid(end));
  exported=read_map(fullfile(root,sprintf('csf_log_band%d.txt',b)));
  values=hdrvdp_ncsf(ppd/2^(b+1),la',p);
  expected=interp1(grid,values,resized);
  n=nnz(actual!=expected); nr=nnz(exported!=resized);
  fprintf('band%d resize_mismatches=%d csf_mismatches=%d max=%.17g expected_class=%s\n',b,nr,n,max(abs(double(actual(:))-double(expected(:)))),class(expected));
  total+=n+nr;
endfor
assert(total==0,sprintf('CSF map mismatches=%d',total));
