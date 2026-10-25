% Actual full-pathway log adaptation -> native per-band resized logs and CSF.
1;
args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;
resize=fopen(fullfile(out,'resize.txt'),'w');bounds=fopen(fullfile(out,'bounds.txt'),'w');assert(resize>=0&&bounds>=0);
pool=fopen(fullfile(out,'pool.txt'),'w');assert(pool>=0);
p=hdrvdp_parse_options('quality',{});assert(p.masking_norm==0&&p.masking_pool_size==3&&!p.do_si_gauss&&!p.do_pu_dilate);
count=0;values=0;records={};
for shape={[80,96],[96,80]}
 hw=shape{1};tag=sprintf('h%d_w%d',hw(1),hw(2));loaded=load(fullfile(source,tag,'trace.mat'));t=loaded.trace;
 input=t.log_La;assert(isa(input,'single'));
 for b=1:numel(t.log_resized)
  resized=t.log_resized{b};csf=t.csf{b};target=size(resized);native=imresize(input,target);
  assert(isequal(typecast(clamp(native,-5,5)(:),'uint32'),typecast(resized(:),'uint32')));
  assert(isa(csf,'single'));rho=30/2^b;
  fprintf(resize,'%d %d %d %d\n',hw,target);fprintf(resize,'%.17g\n',input.');fprintf(resize,'%.17g\n',native.');
  fprintf(bounds,'%d %d %d %d %.17g\n',hw,target,rho);
  fprintf(bounds,'%.17g\n',input.');fprintf(bounds,'%.17g\n',resized.');fprintf(bounds,'%.17g\n',csf.');
  reference=t.bands{b,1}.reference;test=t.bands{b,1}.test;
  raw=min(abs(test),abs(reference));pooled=conv2(raw,ones(3)/9,'same');
  fprintf(pool,'%d %d 1\n',target);fprintf(pool,'%.17g\n',reference.');fprintf(pool,'%.17g\n',test.');
  fprintf(pool,'%.17g\n',raw.');fprintf(pool,'%.17g\n',pooled.');
  count+=1;values+=numel(resized);records{end+1}=struct('input',hw,'target',target,'band',b,'rho',rho);
 endfor
endfor
fclose(resize);fclose(bounds);fclose(pool);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'source',source,'records',{records},...
 'cases',count,'resized_values',values,'ppd',30,'age',24,'precision','native single logs, resized logs and interpolated CSF',...
 'layout','row-major decimal; native imresize bicubic/antialias default',...
 'padding','imresize symmetric edge extension, no FFT padding','DC','none; clamp resized logs[-5,5]')));fclose(f);
