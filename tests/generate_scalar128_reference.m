% Native128 pupil traces and quality-band pooling, isolated from full metric.
1;args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load statistics;
rng(128522026);[x,y]=meshgrid(0:127,0:127);n=16384;
maps={repmat(single(.001),128),repmat(single(10),128),repmat(single(1e4),128),single(10.^(-3+7*(3*x+2*y)/(5*127))),single(10.^(-3+7*rand(128)))};
fi=fopen(fullfile(out,'pupil_input.txt'),'w');fo=fopen(fullfile(out,'pupil_expected.txt'),'w');fprintf(fi,'30 %d\n',n+3);fprintf(fo,'30 4\n');
for ppd=[30,60]
 area=n/ppd^2;
 for pattern=1:5
  image=maps{pattern};geo=geomean(image(:));
  for age=[24,60,60.001]
   dref=pupil_d_unified(geo,area,28);dage=pupil_d_unified(geo,area,age);expected=[geo,dref,dage,dage.^2./dref.^2];assert(isa(expected,'single'));
   fprintf(fi,'%.17g\n',image');fprintf(fi,'%.17g\n',single([area,min(max(age,20),83)-28.58,28-28.58]));fprintf(fo,'%.17g\n',expected);
  endfor
 endfor
endfor
fclose(fi);fclose(fo);
fi=fopen(fullfile(out,'quality_input.txt'),'w');fo=fopen(fullfile(out,'quality_expected.txt'),'w');fprintf(fi,'16 %d\n',2*n);fprintf(fo,'16 1\n');
for k=0:15
 d=single(randn(128)*2^(-10+k));mask=double(rand(128)>.35);
 if k==0,d(:)=0;elseif k==1,d(:)=1;mask(:)=1;
 elseif k==2,d(:)=0;d(13,117)=-1;mask(:)=1;
 elseif k==3,mask(:)=0;
 elseif mod(k,2)==0,mask=rand(128);endif
 masked=d.*mask;expected=sum(abs(masked(:)).^0.8/n).^(1/0.8);assert(isa(expected,'single'));
 fprintf(fi,'%.17g\n',d');fprintf(fi,'%.17g\n',single(mask)');fprintf(fo,'%.17g\n',expected);
endfor
fclose(fi);fclose(fo);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Isolated128 scalar reductions,not scores/full metric','shape',[128,128],'precision','single','pupil_cases',30,'pupil_ppd',[30,60],'ages',[24,60,60.001],'quality_cases',16,'layout','row-major input,column-major native reduction','mask_conversion','mixed double mask rounds tosingle before native multiplication','DC_processing','none')));fclose(f);
fprintf('Native scalar128:pupil120 values,quality16 values\n');
