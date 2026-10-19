% Native rectangular convolution traversal probe, using shared single input.
1;
args=argv(); root=args{1}; addpath(genpath('C:/HDR-VDP-3_Ref'));
[lo0,hi0,lo,bf]=sp0Filters();
for name={'49x65','64x96'}
  f=fopen(fullfile(root,name{1},'input.txt')); d=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  image=single(reshape(v,d(2),d(1))');
  target=corrDn(image,hi0,'reflect1'); radius=4; filter=single(hi0);
  for axis=0:1
    for rx=0:1
      for ry=0:1
        xs=1:9; ys=1:9; if rx,xs=fliplr(xs);endif; if ry,ys=fliplr(ys);endif
        fused=zeros(size(image),'single'); separate=fused;
        for a=1:9
          for b=1:9
            if axis==0, fx=xs(a);fy=ys(b);else,fx=xs(b);fy=ys(a);endif
            yy=abs((0:rows(image)-1)+fy-1-radius); xx=abs((0:columns(image)-1)+fx-1-radius);
            yy(yy>=rows(image))=2*(rows(image)-1)-yy(yy>=rows(image));
            xx(xx>=columns(image))=2*(columns(image)-1)-xx(xx>=columns(image));
            sample=image(yy+1,xx+1); coeff=filter(fy,fx);
            fused=single(double(fused)+double(sample)*double(coeff));
            separate=separate+sample*coeff;
          endfor
        endfor
        hybrid=fused; first=32*floor(rows(image)/32)+1; hybrid(first:end,:)=separate(first:end,:);
        fprintf('%s axis=%d reverse_x=%d reverse_y=%d fused=%d separate=%d hybrid=%d\n',name{1},axis,rx,ry,nnz(fused!=target),nnz(separate!=target),nnz(hybrid!=target));
      endfor
    endfor
  endfor
endfor
