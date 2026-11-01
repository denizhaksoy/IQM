% Goldens for mean/DC -> centered difference -> FFT/hypot/selection.
1;
function words(file,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 v=typecast(a(:),'uint32');f=fopen(file,'w');assert(f>=0);
 fprintf(f,'1 %d\n',numel(v));fprintf(f,'%u\n',v);fclose(f);
endfunction
function equal_bits(a,b)
 assert(isequal(typecast(real(a(:)),'uint32'),typecast(real(b(:)),'uint32')));
 assert(isequal(typecast(imag(a(:)),'uint32'),typecast(imag(b(:)),'uint32')));
endfunction
args=argv();assert(numel(args)==2);root=args{1};out=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
fftw('threads',3);fftw('planner','estimate');f=fopen(fullfile(out,'manifest.txt'),'w');assert(f>=0);
for label={'asymmetric','impulse','hdr_ramp','constant','identity'}
 name=label{1};a=load(fullfile(root,name,'h96_w96','trace.mat'));t=a.trace;
 r=t.visual_reference;v=t.visual_test;rc=t.bands{5,1}.reference;tc=t.bands{5,1}.test;
 input=[reshape(r.base_raw.',[],1);reshape(v.base_raw.',[],1)];
 centered=[r.base_mean;reshape(rc.',[],1);v.base_mean;reshape(tc.',[],1)];
 difference=tc-rc;equal_bits(difference,t.base.difference);
 packed=single(zeros(2,144));packed(1,:)=difference(:).';
 fftw('swisdom','');raw=fft2(difference);dc=raw;dc(1,1)=0;equal_bits(dc,t.base.spectrum);
 spectrum=single(zeros(2,144));ar=real(raw);ai=imag(raw);
 spectrum(1,:)=reshape(ar.',1,[]);spectrum(2,:)=reshape(ai.',1,[]);
 magnitude=abs(dc);selected=find(magnitude(:)==max(magnitude(:)),1);assert(selected==t.base.index);
 expected=[spectrum(:);reshape(magnitude.',[],1);single(selected-1)];assert(numel(expected)==433);
 names={'input','mean','pack','expected'};values={input,centered,packed(:),expected};paths={};
 for k=1:4,paths{k}=fullfile(out,[name '_' names{k} '.txt']);words(paths{k},values{k});paths{k}=strrep(make_absolute_filename(paths{k}),'\','/');endfor
 fprintf(f,'%s 12 12 "%s" "%s" "%s" "%s"\n',name,paths{:});
endfor
fclose(f);
