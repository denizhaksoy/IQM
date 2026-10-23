% Independent official SPYR geometry fixtures before widening public guards.
1;
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);
pkg load image;fftw('threads',3);fftw('swisdom','');
shapes=[64,128;128,64;63,65;65,63;80,96;96,80;127,129;129,127];
records={};
for k=1:rows(shapes)
 h=shapes(k,1);w=shapes(k,2);[x,y]=meshgrid(0:w-1,0:h-1);
 input=single(10+cos(2*pi*x/17)+sin(2*pi*y/23));
 for ppd=[30,60]
  label=sprintf('h%d_w%d_ppd%d',h,w,ppd);folder=fullfile(out,label);assert(mkdir(folder));
  f=fopen(fullfile(folder,'input.txt'),'w');assert(f>=0);
  fprintf(f,'%d %d\n',size(input));fprintf(f,'%.17g\n',input');fclose(f);
  ms=hdrvdp_spyr('sp0Filters');ms=ms.decompose(input,ppd);
  dims=zeros(ms.band_count(),2);
  for b=1:ms.band_count()
   band=ms.get_band(b,1);dims(b,:)=size(band);
   f=fopen(fullfile(folder,sprintf('band%d.txt',b-1)),'w');assert(f>=0);
   fprintf(f,'%d %d\n',size(band));fprintf(f,'%.17g\n',band');fclose(f);
  endfor
  records{end+1}=struct('case',label,'input_shape',[h,w],'ppd',ppd,...
   'band_shapes',dims,'residual_shape',dims(end,:),'pyramid_height',ms.band_count()-2);
  fprintf('%s bands%d residual%dx%d\n',label,ms.band_count(),dims(end,1),dims(end,2));
 endfor
endfor
f=fopen(fullfile(out,'geometry.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('version',hdrvdp_version(),'reference_function',which('hdrvdp_spyr'),...
 'precision','single','layout','row-major text','band_normalization','get_band:raw/2^(band-1)','cases',numel(records),...
 'scope','Native SPYR geometry and band fixtures only;no GPU or full quality claim')));fclose(f);
