1;
function values=read_export(path)
  fid=fopen(path,'r'); assert(fid>=0); dims=fscanf(fid,'%d',2);
  raw=single(fscanf(fid,'%f')); fclose(fid); assert(numel(raw)==prod(dims));
  values=reshape(raw,dims(2),dims(1))';
end
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rand('state',71439); count=262144;
a=single((1+rand(count,1)).*2.^(-149+219*rand(count,1)));
b=single((1+rand(count,1)).*2.^(-149+189*rand(count,1)));
a(1:2:end)=-a(1:2:end);
a(1:5)=single([2^-149,2^-126,2^-126-2^-149,0,2^-149]);
b(1:5)=single([1,.5,2,-1,.5]); offset=6;
for k=2:numel(args)
  root=args{k};
  for label={'ref','test'}
    name=label{1}; raw=read_export(fullfile(root,[name '_la_stage1.txt']));
    spectrum=complex(raw(:,1:2:end),raw(:,2:2:end));
    if strcmp(strtrim(fileread(fullfile(root,'fft_axis_order.txt'))),'column'), spectrum=spectrum.'; end
    filter=read_export(fullfile(root,[name '_la_filter.txt']));
    inputs=[real(spectrum(:));imag(spectrum(:))]; multipliers=[filter(:);filter(:)];
    n=numel(inputs); assert(offset+n-1<=count);
    a(offset:offset+n-1)=inputs; b(offset:offset+n-1)=multipliers; offset+=n;
  end
end
expected=[a.*b,zeros(count,1,'single')]; input=[a,b];
for entry={'input128.txt','forward128.txt'}
  data=input; if strcmp(entry{1},'forward128.txt'), data=expected; end
  fid=fopen(fullfile(output,entry{1}),'w'); assert(fid>=0);
  fprintf(fid,'2048 256\n'); fprintf(fid,'%.9g\n',data'); fclose(fid);
end
fprintf('Native binary32 multiplication: %d pairs, %d spectral pairs\n',count,offset-6);
