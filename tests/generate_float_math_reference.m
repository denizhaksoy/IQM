1;
function values=read_export(path)
  fid=fopen(path,'r'); assert(fid>=0); dims=fscanf(fid,'%d',2);
  raw=single(fscanf(fid,'%f')); fclose(fid); assert(numel(raw)==prod(dims));
  values=reshape(raw,dims(2),dims(1))';
end
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
seed=71439; sources={};
for a=2:numel(args)
  if strncmp(args{a},'--seed=',7), seed=str2double(args{a}(8:end));
  else, sources{end+1}=args{a}; end
end
assert(isfinite(seed)); rand('state',seed); count=65536;
logInput=single(10.^(-6+16*rand(count,1)));
expInput=single(-80+160*rand(count,1));
logInput(1:4)=single([1,1+eps('single'),1-eps('single'),1e-6]);
expInput(1:4)=single([0,1,-1,log(10)]);
offset=5;
for a=1:numel(sources)
  root=sources{a};
  for label={'ref','test'}
    name=label{1};
    adapt=read_export(fullfile(root,[name '_adapt_input.txt']));
    inverse=read_export(fullfile(root,[name '_la_stage3.txt']));
    inverse=inverse(:,1:2:end);
    storage=strtrim(fileread(fullfile(root,'fft_inverse_storage.txt')));
    if strcmp(storage,'column'), inverse=inverse.'; end
    samples=numel(adapt); assert(offset+samples-1<=count);
    logInput(offset:offset+samples-1)=max(adapt(:),single(1e-6));
    crop=inverse(1:64,1:64); expInput(offset:offset+samples-1)=crop(:);
    offset+=samples;
  end
end
expected=[log(logInput),exp(expInput)]; inputs=[logInput,expInput];
for entry={'input128.txt','forward128.txt'}
  data=inputs; if strcmp(entry{1},'forward128.txt'), data=expected; end
  fid=fopen(fullfile(output,entry{1}),'w'); assert(fid>=0);
  fprintf(fid,'512 256\n'); fprintf(fid,'%.9g\n',data'); fclose(fid);
end
fprintf('Native single log/exp reference: %d pairs, %d exported pixel pairs\n',count,offset-5);
