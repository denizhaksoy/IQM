% Official SPYR and base-band goldens, preserving signed-zero words.
1;
function same(a,b)
 assert(isequal(typecast(real(a(:)),'uint32'),typecast(real(b(:)),'uint32')));
 assert(isequal(typecast(imag(a(:)),'uint32'),typecast(imag(b(:)),'uint32')));
endfunction
function words(path,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 v=typecast(a(:),'uint32');f=fopen(path,'w');assert(f>=0);fprintf(f,'1 %d\n',numel(v));fprintf(f,'%u\n',v);fclose(f);
endfunction
args=argv();assert(numel(args)==2||numel(args)==3);source=args{1};out=args{2};hw=[96,96];
if numel(args)==3,assert(any(strcmp(args{3},{'80x80','80x96','96x80','96x96','128x128'})));hw=sscanf(args{3},'%dx%d').';endif
count=prod(hw/8);assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;
fftw('threads',3);fftw('planner','estimate');loaded=load(fullfile(source,sprintf('h%d_w%d',hw),'trace.mat'));t=loaded.trace;
[lo0,hi0,lo,bf]=sp0Filters();stages=cell(1,11);means={};
for label={'reference','test'}
 name=label{1};P=t.(['P_' name]);p=hdrvdp_spyr('sp0Filters');p=p.decompose(P,30);
 high=corrDn(P,hi0,'reflect1');same(high,p.get_band(1,1));a=corrDn(P,lo0,'reflect1');low={a};bands={high};
 for level=1:3
  band=corrDn(a,reshape(bf,9,9),'reflect1')/(2^level);same(band,p.get_band(level+1,1));bands{end+1}=band;
  a=corrDn(a,lo,'reflect1',[2,2],[1,1]);if level==3,a=a/16;endif;low{end+1}=a;
 endfor
 same(a,p.get_band(5,1));v=t.(['visual_' name]);same(a,v.base_raw);
 center=a-mean(a(:));same(center,t.bands{5,1}.(name));
 arrays=[low,bands];for j=1:8,stages{j}=[stages{j};reshape(arrays{j}.',[],1)];endfor
 means{end+1}=[mean(a(:));reshape(center.',[],1)];
endfor
stages{9}=[means{1};means{2}];difference=t.bands{5,1}.test-t.bands{5,1}.reference;same(difference,t.base.difference);
packed=single(zeros(2,count));packed(1,:)=difference(:).';stages{10}=packed(:);
fftw('swisdom','');raw=fft2(difference);dc=raw;dc(1,1)=0;same(dc,t.base.spectrum);
ar=real(raw);ai=imag(raw);spectrum=single(zeros(2,count));spectrum(1,:)=reshape(ar.',1,[]);spectrum(2,:)=reshape(ai.',1,[]);
mag=abs(dc);index=find(mag(:)==max(mag(:)),1);assert(index==t.base.index);
stages{11}=[spectrum(:);reshape(mag.',[],1);single(index-1)];
total=0;for j=1:11,words(fullfile(out,sprintf('stage%d.txt',j-1)),stages{j});total+=numel(stages{j});endfor
assert(total==473*count+3);f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fprintf(f,'Official%dx%d SPYR/base replay:11 stages/%d words;all bands and base FFT/DC/index match captured full reference.',hw(1),hw(2),total);fclose(f);
