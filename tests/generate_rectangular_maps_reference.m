% Native rectangular masking and imresize component fixtures;no GPU inputs.
1;
function write_rows(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',size(a));fprintf(f,'%.17g\n',a');fclose(f);
endfunction
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;
p=hdrvdp_parse_options('quality',{});assert(p.masking_norm==0&&p.masking_pool_size==3);
rng(6412832026);manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);
float_mask_values=0;float_resize_values=0;double_values=0;
for orientation=0:1
 hs=[64,32,16,8];ws=2*hs;if orientation,hs=2*hs;ws=ws/2;endif
 for level=1:4
  h=hs(level);w=ws(level);n=h*w;tag=sprintf('mask_h%d_w%d',h,w);folder=fullfile(out,tag);assert(mkdir(folder));
  input=zeros(16,2*n,'single');expected=input;[x,y]=meshgrid(0:w-1,0:h-1);
  for k=0:15
   a=single(randn(h,w)*2^(-12+2*k));b=single(randn(h,w)*2^(-12+2*k));
   if k==0,a(:)=0;b(:)=0;elseif k==1,a(:)=-3;b(:)=2;
   elseif k==2,a(:)=0;b(:)=0;a(1,1)=1;b(1,1)=-2;
   elseif k==3,a(:)=0;b(:)=0;a(h,w-1)=-1;b(h,w-1)=2;
   elseif k==4,a=single(mod(3*x+7*y,251)-125);b=-a*single(.75);endif
   normalization=2^mod(k,5);raw=min(abs(a/normalization),abs(b/normalization));
   pooled=conv2(raw,ones(3)/9,'same');assert(isa(raw,'single')&&isa(pooled,'single'));
   input(k+1,:)=[a'(:);b'(:)]';expected(k+1,:)=[raw'(:);pooled'(:)]';
  endfor
  write_rows(fullfile(folder,'input.txt'),input);write_rows(fullfile(folder,'expected.txt'),expected);
  fprintf(manifest,'mask %s %d %d %d %d 16\n',tag,h,w,h,w);float_mask_values+=numel(expected);
 endfor
 pairs=[1,1;1,2;1,3;1,4;2,1;2,3;3,2;3,4;4,3;4,1];
 for pair=pairs'
  h=hs(pair(1));w=ws(pair(1));nh=hs(pair(2));nw=ws(pair(2));
  tag=sprintf('float_h%d_w%d_to_h%d_w%d',h,w,nh,nw);folder=fullfile(out,tag);assert(mkdir(folder));
  input=zeros(8,h*w,'single');expected=zeros(8,nh*nw,'single');[x,y]=meshgrid(0:w-1,0:h-1);
  for k=0:7
   a=single((rand(h,w)-.5)*10);
   if k==0,a(:)=0;elseif k==1,a(:)=single(1.25);
   elseif k==2,a(:)=0;a(1,1)=single(3);a(h,w)=single(-2);
   elseif k==3,a=single((-1).^(x+y));
   elseif k==4,a=single(mod(3*x+7*y,19)-9);
   elseif k==6,a=single(10.^(-3+7*(x/(w-1)+y/(h-1))/2));endif
   b=imresize(a,[nh,nw]);assert(isa(b,'single'));
   input(k+1,:)=a'(:)';expected(k+1,:)=b'(:)';
  endfor
  write_rows(fullfile(folder,'input.txt'),input);write_rows(fullfile(folder,'expected.txt'),expected);
  fprintf(manifest,'float %s %d %d %d %d 8\n',tag,h,w,nh,nw);float_resize_values+=numel(expected);
 endfor
 h=hs(1);w=ws(1);[x,y]=meshgrid(0:w-1,0:h-1);input=zeros(8,h*w);
 masks=cell(1,8);
 for k=0:7
  a=double(rand(h,w)>.25);
  if k==0,a(:)=0;elseif k==1,a(:)=1;elseif k==2,a(:)=0;a(1,1)=1;
  elseif k==3,a(:)=0;a(h,w-1)=1;elseif k==4,a=double(mod(x+y,2)==0);
  elseif k==5,a=double(mod(3*x+7*y,19)<7);elseif k==6,a=double(rand(h,w)>.8);endif
  masks{k+1}=a;input(k+1,:)=a'(:)';
 endfor
 for level=1:4
  nh=hs(level);nw=ws(level);tag=sprintf('double_h%d_w%d_to_h%d_w%d',h,w,nh,nw);
  folder=fullfile(out,tag);assert(mkdir(folder));expected=zeros(8,nh*nw);
  for k=1:8,b=imresize(masks{k},[nh,nw]);assert(isa(b,'double'));expected(k,:)=b'(:)';endfor
  write_rows(fullfile(folder,'input.txt'),input);write_rows(fullfile(folder,'expected.txt'),expected);
  fprintf(manifest,'double %s %d %d %d %d 8\n',tag,h,w,nh,nw);double_values+=numel(expected);
 endfor
endfor
fclose(manifest);f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('scope','Native rectangular masking/float resize/double binary-mask resize;no GPU pass',...
 'version',hdrvdp_version(),'Octave',version(),'imresize',which('imresize'),'conv2',which('conv2'),...
 'mask_float_values',float_mask_values,'resize_float_values',float_resize_values,'binary64_values',double_values,...
 'layout','row-major;input/expected rows are independent cases','padding','pool zero outside;resize native default bicubic/antialias/symmetric',...
 'DC_processing','none')));fclose(f);
fprintf('Native maps:mask=%d floatResize=%d binary64=%d\n',float_mask_values,float_resize_values,double_values);
