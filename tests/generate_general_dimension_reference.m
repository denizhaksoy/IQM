% Native geometry/FFT-plan frontier and exact arbitrary-size padding fixtures.
1;
function write_words(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',size(a));
 fprintf(f,'%u\n',typecast(reshape(a.',[],1),'uint32'));fclose(f);
endfunction
function write_complex_words(path,a,column_order)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),2*columns(a));
 if column_order,v=a(:);else,v=reshape(a.',[],1);endif
 packed=zeros(2*numel(a),1,'uint32');packed(1:2:end)=typecast(v,'uint32');
 fprintf(f,'%u\n',packed);fclose(f);
endfunction
function write_pfm(path,a)
 f=fopen(path,'wb','ieee-le');assert(f>=0);fprintf(f,'Pf\n%d %d\n-1.0\n',columns(a),rows(a));
 fwrite(f,flipud(a).','single');fclose(f);
endfunction
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;pkg load statistics;
fftw('threads',3);fftw('planner','estimate');
shapes=[63,65;65,63;80,96;96,80;127,129;129,127;192,256;256,192];
manifest=fopen(fullfile(out,'padding_manifest.txt'),'w');assert(manifest>=0);records={};
for s=1:rows(shapes)
 h=shapes(s,1);w=shapes(s,2);[x,y]=meshgrid(0:w-1,0:h-1);
 folder=fullfile(out,sprintf('h%d_w%d',h,w));assert(mkdir(folder));
 reference=single(10)+single(.01)*single(mod(3*x+7*y+x.*y,101)-50);
 for pattern=1:3
  a=reference;
  if pattern==2,a(:)=single(0);a(1,1)=single(-7);a(h,w)=single(11);a(1,w)=typecast(uint32(2147483648),'single');endif
  if pattern==3,a=single(10.^(-4+9*(3*x/(w-1)+2*y/(h-1))/5));endif
  write_words(fullfile(folder,sprintf('input%d.txt',pattern)),a);
  for mode=1:2
   method='symmetric';if mode==2,method='replicate';endif
   padded=padarray(a,[h,w],method,'post');assert(isequal(size(padded),[2*h,2*w]));
   for layout=0:1
    name=sprintf('p%d_m%d_l%d',pattern,mode,layout);
    write_complex_words(fullfile(folder,[name '.txt']),padded,layout==1);
    fprintf(manifest,'h%d_w%d %d %d %d %d %d %s\n',h,w,h,w,pattern,mode,layout,[name '.txt']);
   endfor
  endfor
 endfor
 test=reference;ys=floor(h*.25)+1:floor(h*.65);xs=floor(w*.55)+1:floor(w*.85);
 test(ys,xs)=reference(ys,xs)*single(1.4)+single(.25);
 write_pfm(fullfile(folder,'reference.pfm'),reference);write_pfm(fullfile(folder,'test.pfm'),test);
 fftw('swisdom','');result=hdrvdp3('quality',test,reference,'luminance',30,{'use_gpu',false,'age',24,'quiet',true});
 assert(isfinite(result.Q)&&isfinite(result.Q_JOD));save(fullfile(folder,'official_result.mat'),'result');
 f=fopen(fullfile(folder,'scores.txt'),'w');fprintf(f,'%.17g %.17g\n',result.Q,result.Q_JOD);fclose(f);
 f=fopen(fullfile(folder,'native_single_wisdom.txt'),'w');fputs(f,fftw('swisdom'));fclose(f);
 pyramid=hdrvdp_spyr('sp0Filters');pyramid=pyramid.decompose(reference,30);dims=zeros(pyramid.band_count(),2);
 for b=1:pyramid.band_count(),dims(b,:)=size(pyramid.get_band(b,1));endfor
 records{end+1}=struct('shape',[h,w],'fft_shape',[2*h,2*w],'fft_height_factors',factor(2*h),...
  'fft_width_factors',factor(2*w),'band_shapes',dims,'residual_samples',prod(dims(end,:)),...
  'Q',result.Q,'Q_JOD',result.Q_JOD);
 fprintf('Frontier%dx%d FFT%dx%d residual%dx%d Q=%.17g JOD=%.17g\n',h,w,2*h,2*w,dims(end,1),dims(end,2),result.Q,result.Q_JOD);
endfor
fclose(manifest);f=fopen(fullfile(out,'frontier.json'),'w');fputs(f,jsonencode(records));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',hdrvdp_version(),...
 'scope','Native full-quality references and isolated padding;no general GPU quality claim',...
 'ppd',30,'age',24,'threads',3,'planner',fftw('planner'),'precision','single',...
 'padding','post to exactly2H x2W;symmetric MTF,replicate LA;no power-of-two rounding',...
 'layout','row-major input;row/column-major complex output;explicit positive imaginary zero',...
 'DC_processing','none in padding','cases',96)));fclose(f);
