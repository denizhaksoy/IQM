1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
pkg load image; addpath(genpath('C:/HDR-VDP-3_Ref'));
p=hdrvdp_parse_options('quality',{}); p.age=24;
la=logspace(-5,5,256); grid=log10(la); fid=fopen(fullfile(output,'bounds.txt'),'w'); assert(fid>=0);
[x,y]=meshgrid(0:63,0:63); count=0; clipped=0;
for pattern=1:2
  if pattern==1, input=single(10*(x>=32)-5); else input=single(10*mod(floor(x/8)+floor(y/16),2)-5); endif
  for target={[64,64],[32,32],[16,16],[8,8],[79,69]}
    sz=target{1}; raw=imresize(input,sz); resized=clamp(raw,-5,5); clipped+=nnz(raw!=resized);
    rho=3.75; values=hdrvdp_ncsf(rho,la',p); csf=interp1(grid,values,resized);
    fprintf(fid,'%d %d %d %d %.17g\n',size(input),sz,rho);
    for matrix={input,resized,csf}, fprintf(fid,'%.9g ',matrix{1}'); fprintf(fid,'\n'); endfor
    count+=1;
  endfor
endfor
fclose(fid); fprintf('CSF bounds cases=%d clipped_pixels=%d\n',count,clipped); assert(clipped>0);
