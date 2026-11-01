% Capture original official base bands, their native means and centered bands.
1;
function words(file,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 v=reshape(a.',[],1);v=typecast(v,'uint32');
 f=fopen(file,'w');assert(f>=0);fprintf(f,'1 %d\n',numel(v));fprintf(f,'%u\n',v);fclose(f);
endfunction
args=argv();assert(numel(args)==2);root=args{1};out=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);records={};
for label={'asymmetric','impulse','hdr_ramp','constant','identity'}
 name=label{1};a=load(fullfile(root,name,'h96_w96','trace.mat'));t=a.trace;
 for image=1:2
  if image==1,v=t.visual_reference;centered=t.bands{5,1}.reference;else,v=t.visual_test;centered=t.bands{5,1}.test;endif
  assert(isequal(size(v.base_raw),[12,12])&&isa(v.base_mean,'single'));
  assert(isequal(typecast(reshape((v.base_raw-v.base_mean).',[],1),'uint32'),typecast(reshape(centered.',[],1),'uint32')));
  tag=sprintf('%s_%d',name,image);input=fullfile(out,[tag '_input.txt']);expected=fullfile(out,[tag '_expected.txt']);
  words(input,v.base_raw);words(expected,[v.base_mean;reshape(centered.',[],1)]);
  fprintf(manifest,'%s "%s" "%s"\n',tag,strrep(make_absolute_filename(input),'\','/'),strrep(make_absolute_filename(expected),'\','/'));
  records{end+1}=struct('case',tag,'count',144,'native_mean',v.base_mean,'min',min(v.base_raw(:)),'max',max(v.base_raw(:)));
 endfor
endfor
fclose(manifest);f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
