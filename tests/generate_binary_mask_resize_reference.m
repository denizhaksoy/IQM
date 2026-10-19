1;
args=argv(); output=args{1}; assert(!exist(output,'dir')); assert(mkdir(output));
side=64; if numel(args)>1, side=str2double(args{2}); endif
assert(side==32||side==64);
pkg load image; rand('state',71805); [x,y]=meshgrid(0:side-1,0:side-1);
f=fopen(fullfile(output,'mask_resize.txt'),'w'); assert(f>=0);
for k=0:63
  mask=double(rand(side,side)<(1+mod(k,9))/10);
  if k==0, mask(:)=0;
  elseif k==1, mask(:)=1;
  elseif k>=2&&k<=5, mask(:)=0; corners=[1,1;1,side;side,1;side,side];mask(corners(k-1,1),corners(k-1,2))=1;
  elseif k==6, mask=double(mod(x+y,2)==0);
  elseif k==7, mask=double(x<side/2);
  elseif k==8, mask=double(y<side/2);
  elseif k==9, mask=double(mod(x,5)==0);
  elseif k==10, mask=double(mod(y,7)==0);
  elseif k==11, mask(:)=0; mask(side/2,side/2+1)=1;
  endif
  for n=side./2.^(0:log2(side/8))
    expected=imresize(mask,[n,n]); assert(isa(expected,'double'));
    fprintf(f,'%d %d %d %d\n',side,side,n,n); fprintf(f,'%.17g\n',mask'); fprintf(f,'%.17g\n',expected');
  endfor
endfor
fclose(f); fprintf('Native binary mask resize: side=%d cases=%d double_outputs=%d\n',side,64*(1+log2(side/8)),64*sum((side./2.^(0:log2(side/8))).^2));
