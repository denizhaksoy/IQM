% Use actual resized base-adaptation logs from complete native pathways.
1;
function save_words(path,n,words)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'1 %d\n',n);fprintf(f,'%u\n',words);fclose(f);
endfunction
args=argv();assert(numel(args)==2);out=args{1};source=args{2};
assert(!exist(out,'dir'));assert(mkdir(out));
metadata=jsondecode(fileread(fullfile(source,'metadata.json')));assert(metadata.version==3.07);
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);
for k=1:numel(metadata.records)
 shape=metadata.records(k).shape;tag=sprintf('h%d_w%d',shape(1),shape(2));
 loaded=load(fullfile(source,tag,'trace.mat'));trace=loaded.trace;
 logs=trace.log_resized{end};assert(isa(logs,'single')&&numel(logs)==120);
 m=mean(logs(:));assert(isequal(typecast(m,'uint32'),typecast(single(sum(double(logs(:)))/120),'uint32')));
 power=single(10).^m;assert(isequal(typecast(power,'uint32'),typecast(trace.base.L_mean,'uint32')));
 input=strrep(fullfile(out,[tag '_input.txt']),'\','/');expected=strrep(fullfile(out,[tag '_expected.txt']),'\','/');
 save_words(input,120,typecast(reshape(logs.',[],1),'uint32'));
 save_words(expected,2,typecast([m;power],'uint32'));
 fprintf(manifest,'%s "%s" "%s"\n',tag,input,expected);
endfor
fclose(manifest);
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('scope','Two actual full-pathway base logmaps;not complete GPU quality',...
 'source',source,'version',3.07,'ppd',30,'age',24,'layout','row-major120 -> mean,pow10',...
 'precision','native single','padding','none in component','DC','none')));fclose(f);
