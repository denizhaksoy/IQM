1;
function write_words(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),columns(a));
 fprintf(f,'%u\n',typecast(reshape(a.',[],1),'uint32'));fclose(f);
endfunction
args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;
[lo0,hi0,lo,bf]=sp0Filters();write_words(fullfile(out,'lo0.txt'),single(lo0));write_words(fullfile(out,'lo.txt'),single(lo));
write_words(fullfile(out,'hi0.txt'),single(hi0));write_words(fullfile(out,'orient.txt'),single(reshape(bf,9,9)));
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);records={};
for shape={[80,96],[96,80]}
 hw=shape{1};tag=sprintf('h%d_w%d',hw(1),hw(2));loaded=load(fullfile(source,tag,'trace.mat'));t=loaded.trace;
 for label={'reference','test'}
  name=[tag '_' label{1}];folder=fullfile(out,name);assert(mkdir(folder));a=t.(['P_' label{1}]);write_words(fullfile(folder,'input.txt'),a);
  p=hdrvdp_spyr('sp0Filters');p=p.decompose(a,30);
  high=corrDn(a,hi0,'reflect1');assert(isequal(high,p.get_band(1,1)));write_words(fullfile(folder,'high.txt'),high);
  a=corrDn(a,lo0,'reflect1');write_words(fullfile(folder,'stage0.txt'),a);
  for level=1:3
   orientation=corrDn(a,reshape(bf,9,9),'reflect1')/(2^level);
   assert(isequal(typecast(orientation(:),'uint32'),typecast(p.get_band(level+1,1)(:),'uint32')));
   write_words(fullfile(folder,sprintf('orient%d.txt',level-1)),orientation);
   a=corrDn(a,lo,'reflect1',[2,2],[1,1]);
   if level==3,a=a/16;endif
   write_words(fullfile(folder,sprintf('stage%d.txt',level)),a);
  endfor
  raw=p.get_band(p.band_count(),1);assert(isequal(typecast(a(:),'uint32'),typecast(raw(:),'uint32')));
  centered=raw-mean(raw(:));assert(isequal(typecast(centered(:),'uint32'),typecast(t.bands{end,1}.(label{1})(:),'uint32')));
  fprintf(manifest,'%s %d %d "%s"\n',name,hw(1),hw(2),strrep(folder,'\','/'));
  records{end+1}=struct('tag',name,'shape',hw,'residual_shape',size(a),'raw_identity',true,'centered_identity',true);
 endfor
endfor
fclose(manifest);sources={'buildSpyr','buildSpyrLevs','corrDn','rconv2','sp0Filters'};hashes=struct();
for k=1:numel(sources),n=sources{k};hashes.(n)=hash('sha256',fileread(which(n)));endfor
metadata=struct('version',3.07,'source',source,'records',{records},'native_source_hashes',hashes,...
 'precision','Float32 images and once-rounded Float32 coefficients; native corrDn mixed arithmetic',...
 'ppd',30,'age',24,'layout','row-major uint32 Float32 words',...
 'padding','corrDn reflect1 at each stage; start[1,1]; step1 then three step2',...
 'normalization','only final raw base band /16; no DC subtraction in lowpass chain');
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(metadata));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Four actual pathway lowpass chains reproduce native raw and centered SPYR base bands bitwise.');fclose(f);
