% Native mean/DC -> paired difference -> FFT/magnitude/selection goldens.
1;
function words(file,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 v=typecast(a(:),'uint32');f=fopen(file,'w');assert(f>=0);
 fprintf(f,'1 %d\n',numel(v));fprintf(f,'%u\n',v);fclose(f);
endfunction
function equal_bits(a,b)
 assert(isequal(size(a),size(b))&&strcmp(class(a),class(b)));
 assert(isequal(typecast(real(a(:)),'uint32'),typecast(real(b(:)),'uint32')));
 assert(isequal(typecast(imag(a(:)),'uint32'),typecast(imag(b(:)),'uint32')));
endfunction
args=argv();assert(numel(args)>=3&&numel(args)<=5);root=args{1};old=args{2};out=args{3};shapes={[80,80],[80,96],[96,80],[96,96]};labels={'asymmetric','impulse','hdr_ramp','constant','identity'};
if numel(args)>=4,assert(strcmp(args{4},'--square128'));shapes={[128,128]};endif
if numel(args)==5,assert(strcmp(args{5},'--competing-axes'));labels{end+1}='competing_axes';endif
assert(!exist(out,'dir'));assert(mkdir(out));
fftw('threads',3);fftw('planner','estimate');manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);records={};
limits=fopen(fullfile(out,'limits_manifest.txt'),'w');assert(limits>=0);
for shape=shapes
 hw=shape{1};h=hw(1);w=hw(2);bh=h/8;bw=w/8;n=bh*bw;
 for label=labels
  name=label{1};
  if h==128&&w==128,source=fullfile(root,['native_' name]);
  elseif h==96&&w==96,source=fullfile(old,['native_' name]);
  elseif h==80&&w==80,source=fullfile(root,['native_' name]);
  else,source=fullfile(root,['native_rect_' name]);endif
  a=load(fullfile(source,sprintf('h%d_w%d',h,w),'trace.mat'));t=a.trace;
  r=t.visual_reference;v=t.visual_test;rc=t.bands{5,1}.reference;tc=t.bands{5,1}.test;
  assert(isequal(size(rc),[bh,bw])&&isa(rc,'single'));
  equal_bits(r.base_raw-r.base_mean,rc);equal_bits(v.base_raw-v.base_mean,tc);
  input=[reshape(r.base_raw.',[],1);reshape(v.base_raw.',[],1)];
  centered=[r.base_mean;reshape(rc.',[],1);v.base_mean;reshape(tc.',[],1)];
  difference=tc-rc;equal_bits(difference,t.base.difference);
  packed=single(zeros(2,n));packed(1,:)=difference(:).';
  fftw('swisdom','');raw=fft2(difference);dc=raw;dc(1,1)=0;equal_bits(dc,t.base.spectrum);
  spectrum=single(zeros(2,n));ar=real(raw);ai=imag(raw);
  spectrum(1,:)=reshape(ar.',1,[]);spectrum(2,:)=reshape(ai.',1,[]);
  magnitude=abs(dc);selected=find(magnitude(:)==max(magnitude(:)),1);assert(selected==t.base.index);
  expected=[spectrum(:);reshape(magnitude.',[],1);single(selected-1)];
  tag=sprintf('h%d_w%d_%s',h,w,name);names={'input','mean','pack','expected'};values={input,centered,packed(:),expected};paths={};
  for k=1:4,paths{k}=fullfile(out,[tag '_' names{k} '.txt']);words(paths{k},values{k});paths{k}=strrep(make_absolute_filename(paths{k}),'\','/');endfor
  fprintf(manifest,'%s %d %d "%s" "%s" "%s" "%s"\n',tag,bh,bw,paths{:});
  logpath=fullfile(out,[tag '_log.txt']);adaptpath=fullfile(out,[tag '_adapt.txt']);limitpath=fullfile(out,[tag '_limits.bin']);
  logband=t.log_resized{end};assert(isa(logband,'single')&&isequal(size(logband),[bh,bw]));
  logmean=mean(logband(:));adapt=single(10).^logmean;equal_bits(adapt,t.base.L_mean);
  words(logpath,reshape(logband.',[],1));words(adaptpath,[logmean;adapt]);
  lf=fopen(limitpath,'w','ieee-le');assert(lf>=0);assert(fwrite(lf,[double(t.base.frequency),double(t.base.N_nCSF)],'double')==2);fclose(lf);
  fprintf(limits,'%s %d %d "%s" "%s" "%s"\n',tag,bh,bw,strrep(make_absolute_filename(logpath),'\','/'),strrep(make_absolute_filename(adaptpath),'\','/'),strrep(make_absolute_filename(limitpath),'\','/'));
  records{end+1}=struct('case',tag,'source_shape',hw,'base_shape',[bh,bw],'index_zero_based',selected-1,...
   'frequency',t.base.frequency,'adaptation',t.base.L_mean,'noise',t.base.N_nCSF,...
   'zero_frequency_noise_limit',t.base.frequency==0&&isinf(t.base.N_nCSF)&&t.base.N_nCSF>0,...
   'Q',a.baseline.Q,'Q_JOD',a.baseline.Q_JOD);
 endfor
endfor
fclose(manifest);fclose(limits);f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
