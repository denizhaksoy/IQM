% Reproduce the official local Minkowski expression on saved native D_p only.
1;
args=argv();assert(numel(args)==1);root=args{1};official='C:/HDR-VDP-3_Ref/hdrvdp3.m';s=fileread(official);assert(!isempty(strfind(s,'d = sum( abs(X).^p / numel(X) ).^(1/p);')));
metadata=jsondecode(fileread(fullfile(root,'metadata.json')));
for record=1:numel(metadata.records)
 hw=metadata.records(record).shape;folder=fullfile(root,sprintf('h%d_w%d',hw(1),hw(2)));a=load(fullfile(folder,'trace.mat'));t=a.trace;errors=single(zeros(5,1));acc=0;
 for b=1:5
  X=t.masking{b,1}.quality_difference(:);p=.8;errors(b)=sum(abs(X).^p/numel(X)).^(1/p);acc=acc+errors(b)/5;
  assert(isequal(acc,t.masking{b,1}.accumulated_error));
  m=t.masking{b,1};psych=sign(m.difference).*abs(m.difference).^m.pf;assert(all(isfinite(psych(:))));
  f=fopen(fullfile(folder,sprintf('band%d_psychometric.txt',b)),'w');assert(f>=0);fprintf(f,'%d %d\n',rows(psych),columns(psych));fprintf(f,'%.17g\n',psych.');fclose(f);
 endfor
 f=fopen(fullfile(folder,'band_errors.txt'),'w');assert(f>=0);fprintf(f,'5 1\n');fprintf(f,'%.17g\n',errors);fclose(f);
 f=fopen(fullfile(folder,'Q_error.txt'),'w');assert(f>=0);fprintf(f,'1 1\n%.17g\n',acc);fclose(f);
endfor
f=fopen(fullfile(root,'band_error_provenance.txt'),'w');assert(f>=0);fprintf(f,'Official local Minkowski expression;native D_p only;all accumulated errors identical;source SHA256=%s\n',hash('sha256',s));fclose(f);
