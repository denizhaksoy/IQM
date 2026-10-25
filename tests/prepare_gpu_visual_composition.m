% Copy independent goldens and serialize GPU P/log outputs without image math.
1;
function ok=copy_bytes(source,destination)
 f=fopen(source,'rb');assert(f>=0);bytes=fread(f,Inf,'*uint8');fclose(f);f=fopen(destination,'wb');assert(f>=0);assert(fwrite(f,bytes,'uint8')==numel(bytes));fclose(f);ok=true;
endfunction
args=argv();assert(numel(args)==4);out=args{1};gpu=args{2};native=args{3};trace=args{4};assert(!exist(out,'dir'));assert(mkdir(out));
spyr=fullfile(out,'spyr_inputs');logs=fullfile(out,'log_inputs');assert(mkdir(spyr));assert(mkdir(logs));
for name={'lo0.txt','lo.txt','hi0.txt','orient.txt'},assert(copy_bytes(fullfile(native,name{1}),fullfile(spyr,name{1})));endfor
manifest=fopen(fullfile(spyr,'manifest.txt'),'w');assert(manifest>=0);
for hw={[80,96],[96,80]}
 shape=hw{1};tag=sprintf('h%d_w%d',shape(1),shape(2));old=load(fullfile(trace,tag,'trace.mat'));
 for label={'reference','test'}
  n=label{1};name=[tag '_' n];folder=fullfile(spyr,name);assert(mkdir(folder));files=dir(fullfile(native,name,'*.txt'));
  for k=1:numel(files),if !strcmp(files(k).name,'input.txt'),assert(copy_bytes(fullfile(native,name,files(k).name),fullfile(folder,files(k).name)));endif;endfor
  f=fopen(fullfile(gpu,[name '_P.f32']),'rb','ieee-le');assert(f>=0);words=fread(f,Inf,'*uint32');fclose(f);
  assert(isequal(words,typecast(reshape(old.trace.(['P_' n]).',[],1),'uint32')));
  f=fopen(fullfile(folder,'input.txt'),'w');assert(f>=0);fprintf(f,'%d %d\n',shape);fprintf(f,'%u\n',words);fclose(f);
  fprintf(manifest,'%s %d %d "%s"\n',name,shape,strrep(folder,'\','/'));
 endfor
 folder=fullfile(logs,tag);assert(mkdir(folder));assert(copy_bytes(fullfile(gpu,[tag '_log_La.txt']),fullfile(folder,'log_La.txt')));
 assert(copy_bytes(fullfile(trace,tag,'base_selection.txt'),fullfile(folder,'base_selection.txt')));
endfor
fclose(manifest);f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('GPU_source',gpu,'SPYR_native_goldens',native,'trace_goldens',trace,...
 'scope','Runtime P/log from GPU; native maps only goldens; no host filtering or log calculation',...
 'layout','row-major uint32 P input; decimal Float32 log input','DC','none introduced')));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Four GPU P maps bit-identical to native; serialized for SPYR with GPU log maps.');fclose(f);
