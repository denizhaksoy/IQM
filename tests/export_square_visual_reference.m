% Native-only visual goldens for the connected original-image diagnostic.
1;
function write_single(path,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),columns(a));fprintf(f,'%.17g\n',a.');fclose(f);
endfunction
args=argv();assert(numel(args)==2||numel(args)==3);source=args{1};out=args{2};hw=[96,96];
if numel(args)==3,assert(any(strcmp(args{3},{'80x80','96x96','128x128'})));hw=sscanf(args{3},'%dx%d').';endif
assert(!exist(out,'dir'));assert(mkdir(out));
a=load(fullfile(source,sprintf('h%d_w%d',hw),'trace.mat'));t=a.trace;
for label={'reference','test'}
 name=label{1};v=t.(['visual_' name]);p=t.(['P_' name]);
 assert(isequal(typecast(v.P(:),'uint32'),typecast(p(:),'uint32')));
 assert(numel(v.pupil)==4);
 write_single(fullfile(out,[name '_P.txt']),p);
 write_single(fullfile(out,[name '_pupil.txt']),reshape(v.pupil,1,4));
endfor
write_single(fullfile(out,'mean_adaptation.txt'),t.L_mean);
write_single(fullfile(out,'log_adaptation.txt'),t.log_La);
f=fopen(fullfile(out,'source_metadata.json'),'w');assert(f>=0);fputs(f,fileread(fullfile(source,'metadata.json')));fclose(f);
