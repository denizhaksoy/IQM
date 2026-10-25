% Serialize GPU outputs; no host filtering, DC removal or runtime masking.
1;
function words=load_words(path)
 f=fopen(path,'rb','ieee-le');assert(f>=0);words=fread(f,Inf,'*uint32');fclose(f);
endfunction
function save_words(path,words)
 f=fopen(path,'wb','ieee-le');assert(f>=0);fwrite(f,words,'uint32');fclose(f);
endfunction
args=argv();assert(numel(args)==3);out=args{1};gpu=args{2};native=args{3};assert(!exist(out,'dir'));assert(mkdir(out));
pool=fopen(fullfile(out,'pool.txt'),'w');assert(pool>=0);count=0;values=0;
for hw={[80,96],[96,80]}
 shape=hw{1};tag=sprintf('h%d_w%d',shape(1),shape(2));loaded=load(fullfile(native,tag,'trace.mat'));t=loaded.trace;
 folder=fullfile(out,tag);assert(mkdir(folder));
 for b=1:5
  expected=t.bands{b,1};n=numel(expected.reference);
  if b<5,words=load_words(fullfile(gpu,sprintf('%s_reference_stage%d.u32',tag,b+3)));assert(numel(words)==2*n);r=words(1:n);s=words(n+1:end);
  else,words=load_words(fullfile(gpu,[tag '_reference_stage8.u32']));assert(numel(words)==242);r=words(2:121);s=words(123:242);endif
  assert(isequal(r,typecast(reshape(expected.reference.',[],1),'uint32')));
  assert(isequal(s,typecast(reshape(expected.test.',[],1),'uint32')));
  save_words(fullfile(folder,sprintf('band%d_reference.f32',b)),r);save_words(fullfile(folder,sprintf('band%d_test.f32',b)),s);
  raw=min(abs(expected.test),abs(expected.reference));pooled=t.masking{b,1}.self;
  fprintf(pool,'%d %d 1\n',size(expected.reference));fprintf(pool,'%.17g\n',typecast(r,'single'));fprintf(pool,'%.17g\n',typecast(s,'single'));
  fprintf(pool,'%.17g\n',raw.');fprintf(pool,'%.17g\n',pooled.');count+=1;values+=n;
 endfor
endfor
fclose(pool);assert(count==10&&values==35760);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','GPU band serialization; native maps are comparison goldens only',...
 'GPU_source',gpu,'native_golden_source',native,'bands',count,'values_per_reference_or_test',values,...
 'layout','row-major Float32 binary; base means skipped without arithmetic',...
 'normalization','already GPU /1,/2,/4,/8,/16; base already GPU centered','DC','no host subtraction')));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'GPU reference/test bands match native goldens bitwise; runtime pool inputs serialized from GPU dumps.');fclose(f);
