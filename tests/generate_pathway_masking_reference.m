1;
function write_values(path,a)
 f=fopen(path,'w');assert(f>=0&&isreal(a)&&all(isfinite(a(:))));
 fprintf(f,'%d %d\n',rows(a),columns(a));fprintf(f,'%.17g\n',a.');fclose(f);
endfunction
args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);records={};
for shape={[80,96],[96,80]}
 hw=shape{1};tag=sprintf('h%d_w%d',hw(1),hw(2));loaded=load(fullfile(source,tag,'trace.mat'));t=loaded.trace;folder=fullfile(out,tag);assert(mkdir(folder));
 errors=[];accumulated=0;
 for b=1:5
  mask=t.masking{b,1};fields=fieldnames(mask);
  for k=1:numel(fields),n=fields{k};write_values(fullfile(folder,sprintf('band%d_%s.txt',b,n)),mask.(n));endfor
  write_values(fullfile(folder,sprintf('band%d_reference.txt',b)),t.bands{b,1}.reference);
  write_values(fullfile(folder,sprintf('band%d_test.txt',b)),t.bands{b,1}.test);
  write_values(fullfile(folder,sprintf('band%d_csf.txt',b)),t.csf{b});
  psych=sign(mask.difference).*abs(mask.difference).^mask.pf;
  write_values(fullfile(folder,sprintf('band%d_psychometric.txt',b)),psych);
  v=mask.quality_difference(:);error=sum(abs(v).^.8/numel(v)).^(1/.8);errors=[errors;error];
  accumulated=accumulated+error/5;assert(isequal(accumulated,mask.accumulated_error));
  records{end+1}=struct('tag',tag,'band',b,'shape',size(mask.self),'self_class',class(mask.self),...
   'neighbor_class',class(mask.neighbor),'denominator_class',class(mask.denominator),...
   'D_class',class(mask.difference),'Dp_class',class(mask.quality_difference),'comparison_class',class(mask.comparison));
 endfor
 write_values(fullfile(folder,'errors.txt'),errors);write_values(fullfile(folder,'scores.txt'),[loaded.baseline.Q,loaded.baseline.Q_JOD]);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'source',source,'records',{records},...
 'scope','Native masking/excitation/psychometric/quality component fixtures; no full GPU quality claim',...
 'ppd',30,'age',24,'layout','row-major decimal17digits; original classes in records',...
 'padding','native neighbor imresize symmetric extension; other stages elementwise',...
 'DC','bands already normalized; base already centered; no additional DC removal')));fclose(f);
