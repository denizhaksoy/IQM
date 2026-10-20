% Independent PFM decode checks the actual metric exports before comparison.
1;
function image=pfm(path)
  f=fopen(path,'rb');assert(f>=0);assert(strcmp(strtrim(fgetl(f)),'Pf'));
  shape=sscanf(fgetl(f),'%d');assert(numel(shape)==2);scale=str2double(fgetl(f));
  machine='ieee-be';if scale<0,machine='ieee-le';endif
  values=fread(f,prod(shape),'*single',0,machine);assert(numel(values)==prod(shape));assert(isempty(fread(f,1,'uint8')));fclose(f);
  image=flipud(reshape(values,shape(1),shape(2))')*single(abs(scale));
endfunction
function image=map(path)
  f=fopen(path);assert(f>=0);shape=fscanf(f,'%d',2);values=fscanf(f,'%f');fclose(f);
  assert(numel(values)==prod(shape));image=single(reshape(values,shape(2),shape(1))');
endfunction
args=argv();root=args{1};count=0;
for k=1:2
  label={'ref','test'}{k};original=pfm(args{k+1});actual=map(fullfile(root,[label '_luminance.txt']));
  assert(isequal(size(original),size(actual)));
  mismatches=nnz(typecast(original(:),'uint32')!=typecast(actual(:),'uint32'));
  assert(mismatches==0);count+=numel(original);
  fprintf('%s original_PFM_bits=%d bit_mismatches=%d min=%.17g max=%.17g\n',label,numel(original),mismatches,min(original(:)),max(original(:)));
endfor
fprintf('PFM/export input values=%d bit_mismatches=0\n',count);
