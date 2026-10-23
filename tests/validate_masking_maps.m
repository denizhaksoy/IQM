1;
function image=read_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=single(reshape(v,dims(2),dims(1))');
endfunction
args=argv(); root=args{1}; pkg load image; addpath(genpath('C:/HDR-VDP-3_Ref'));
addpath(fileparts(mfilename('fullpath')));band_count=hdrvdp_validation_band_count(root);
p=hdrvdp_parse_options('quality',{}); pools=cell(1,band_count); total=0;
for b=0:band_count-1
  r=read_map(fullfile(root,sprintf('gpu_ref_band%d.txt',b)));
  t=read_map(fullfile(root,sprintf('gpu_test_band%d.txt',b)));
  if b==band_count-1
    r=r-mean(r(:)); t=t-mean(t(:));
  endif
  raw=min(abs(r/2^b),abs(t/2^b));
  norm=10^p.masking_norm; F=ones(p.masking_pool_size);
  pools{b+1}=conv2(raw.^norm,F/numel(F),'same').^(1/norm);
  a=read_map(fullfile(root,sprintf('mask_raw_band%d.txt',b)));
  c=read_map(fullfile(root,sprintf('mask_pool_band%d.txt',b)));
  n=nnz(a!=raw); np=nnz(c!=pools{b+1}); total+=n+np;
  fprintf('band%d raw=%d pool=%d pool_max=%.17g\n',b,n,np,max(abs(double(c(:))-double(pools{b+1}(:)))));
endfor
for b=0:band_count-1
  self=pools{b+1}; neighbor=zeros(size(self));
  if b>0, neighbor=max(imresize(pools{b},size(self)),0); endif
  if b<band_count-2, neighbor=neighbor+max(imresize(pools{b+2},size(self)),0); endif
  q=10^p.mask_q;
  expected=10^p.mask_self*abs(self).^q + 10^p.mask_xo*abs(0).^q + 10^p.mask_xn*abs(neighbor).^q;
  a=read_map(fullfile(root,sprintf('mask_neighbor_band%d.txt',b)));
  c=read_map(fullfile(root,sprintf('mask_denominator_band%d.txt',b)));
  n=nnz(a!=neighbor); nd=nnz(c!=expected); total+=n+nd;
  fprintf('band%d neighbor=%d denominator=%d max=%.17g\n',b,n,nd,max(abs(double(c(:))-double(expected(:)))));
endfor
assert(total==0,sprintf('Masking mismatches=%d',total));
