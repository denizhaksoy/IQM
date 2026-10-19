% Measure native single corrDn arithmetic paths independently of GPU output.
1;
args=argv(); root=args{1}; addpath(genpath('C:/HDR-VDP-3_Ref'));
[lo0,hi0,lo,bf]=sp0Filters(); bf=reshape(bf,9,9);
f=fopen(fullfile(root,'cases.txt')); names=textscan(f,'%s'); fclose(f);
for k=1:numel(names{1})
  name=names{1}{k};
  f=fopen(fullfile(root,name,'input.txt')); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  image=single(reshape(v,dims(2),dims(1))');
  height=min(3,maxPyrHt(size(image),13));
  current=corrDn(image,lo0,'reflect1');
  inputs={image,image}; filters={hi0,lo0}; labels={'hi0','lo0'};
  for level=0:height-1
    inputs{end+1}=current; filters{end+1}=bf; labels{end+1}=sprintf('orient%d',level);
    inputs{end+1}=current; filters{end+1}=lo; labels{end+1}=sprintf('down%d',level);
    current=corrDn(current,lo,'reflect1',[2,2]);
  endfor
  for s=1:numel(inputs)
    image=inputs{s}; filter=single(filters{s}); n=rows(filter); radius=floor(n/2);
    target=corrDn(image,filters{s},'reflect1');
    fused=zeros(size(image),'single'); separate=fused;
    for fx=n:-1:1
      for fy=n:-1:1
        yy=abs((0:rows(image)-1)+fy-1-radius); xx=abs((0:columns(image)-1)+fx-1-radius);
        yy(yy>=rows(image))=2*(rows(image)-1)-yy(yy>=rows(image));
        xx(xx>=columns(image))=2*(columns(image)-1)-xx(xx>=columns(image));
        sample=image(yy+1,xx+1); coeff=filter(fy,fx);
        fused=single(double(fused)+double(sample)*double(coeff));
        separate=separate+sample*coeff;
      endfor
    endfor
    fbad=sum(fused!=target,2); sbad=sum(separate!=target,2);
    fprintf('%s %s input=%dx%d fused_neq=%d separate_neq=%d neither=%d\n',name,labels{s},size(image),sum(fbad),sum(sbad),nnz((fused!=target)&(separate!=target)));
    fprintf(' fused_bad_per_row:'); fprintf(' %d',fbad); fprintf('\n separate_bad_per_row:'); fprintf(' %d',sbad); fprintf('\n');
  endfor
endfor
