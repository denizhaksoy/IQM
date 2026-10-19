% Check the actual exported inputs, independently of metric scores.
1;
function image=read_map(path)
  f=fopen(path,'r');assert(f>=0);d=fscanf(f,'%d',2);v=fscanf(f,'%f');fclose(f);
  assert(numel(v)==prod(d));image=single(reshape(v,d(2),d(1))');
endfunction
args=argv();output=args{end};assert(!exist(output,'file'));records={};
for a=1:numel(args)-1
  f=fopen(args{a});assert(f>=0);
  while true
    line=fgetl(f);if !ischar(line),break;endif
    parts=strsplit(line,char(9));assert(numel(parts)==2);name=parts{1};root=parts{2};
    image=read_map(fullfile(root,'ref_luminance.txt'));[h,w]=size(image);
    pattern=strtrim(fileread(fullfile(root,'pattern.txt')));
    if strcmp(pattern,'hdr_ramp')
      assert(image(1,1)==single(1e-3)&&image(end,end)==single(1e4));
      assert(all(diff(image,1,1)(:)>0)&&all(diff(image,1,2)(:)>0));
    elseif strcmp(pattern,'hdr_edges')
      expected=single([.001,.01,.1,1,10,100,1000,10000]);
      assert(isequal(sort(unique(image(:)))',expected));
      assert(numel(unique(image(1,:)))==8);
    elseif strcmp(pattern,'impulse')
      assert(nnz(image!=10)==3);
      assert(image(8,1)==single(10.5));
      assert(image(floor(43*h/64)+1,floor(19*w/64)+1)==single(9.5));
      assert(image(h-2,w)==single(10.25));
    else
      error('Unsupported geometry pattern');
    endif
    records{end+1}=struct('name',name,'pattern',pattern,'height',h,'width',w,'minimum',double(min(image(:))),'maximum',double(max(image(:))),'levels',numel(unique(image(:))),'non_background',nnz(image!=10));
    fprintf('%s %dx%d input_geometry=PASS min=%.17g max=%.17g levels=%d\n',name,h,w,min(image(:)),max(image(:)),numel(unique(image(:))));
  endwhile
  fclose(f);
endfor
f=fopen(output,'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
