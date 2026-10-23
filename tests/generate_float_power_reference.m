1;
function values=read_map(root,name)
  f=fopen(fullfile(root,name),'r'); assert(f>=0); dims=fscanf(f,'%d',2);
  values=single(fscanf(f,'%f')); fclose(f); assert(numel(values)==prod(dims));
endfunction
% Isolated positive finite power probe; no production backend is selected.
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
seed=71440; sources={};
for a=2:numel(args)
  if strncmp(args{a},'--seed=',7), seed=str2double(args{a}(8:end));
  else, sources{end+1}=args{a}; endif
endfor
rand('state',seed); count=65536;
x=single(10.^(-6+12*rand(count,1)));
exponents=single([10^.108934275615,10^.3424,2*10^.3424,10^.34/10^.3424,.8,1/.8,1.2812,.41,2]);
y=exponents(1+mod((0:count-1)',numel(exponents))); y=y(:);
x(1:5)=single([0,1,38.792343139648438,1e-6,10000]);
y(1:5)=single([2,2,2,.41,1.2812]);
offset=6;
for a=1:numel(sources)
  root=sources{a}; p=10^.3424; pf=10^.34/p;
  for b=0:4
    r=read_map(root,sprintf('gpu_ref_band%d.txt',b));
    t=read_map(root,sprintf('gpu_test_band%d.txt',b));
    if b==4, r-=mean(r(:)); t-=mean(t(:)); endif
    difference=t/single(2^b)-r/single(2^b);
    values={read_map(root,sprintf('mask_pool_band%d.txt',b)), ...
      read_map(root,sprintf('mask_neighbor_band%d.txt',b)), ...
      abs(read_map(root,sprintf('quality_difference_band%d.txt',b))), ...
      1./read_map(root,sprintf('csf_band%d.txt',b)),abs(difference), ...
      abs(read_map(root,sprintf('quality_difference_band%d.txt',b)))};
    powers=[10^.108934275615,10^.108934275615,pf,2*p,p,.8];
    for k=1:numel(values)
      v=values{k}(:); n=numel(v); assert(offset+n-1<=count);
      x(offset:offset+n-1)=v; y(offset:offset+n-1)=single(powers(k)); offset+=n;
    endfor
  endfor
endfor
expected=x.^y;
assert(isa(expected,'single') && all(isfinite(expected)));
for entry={'input128.txt','forward128.txt'}
  data=[x,y]; if strcmp(entry{1},'forward128.txt'), data=[expected,expected]; end
  fid=fopen(fullfile(output,entry{1}),'w'); assert(fid>=0);
  fprintf(fid,'512 256\n'); fprintf(fid,'%.9g\n',data'); fclose(fid);
end
fprintf('Native positive single power reference: %d pairs, seed%d, exported arguments%d\n',count,seed,offset-6);
