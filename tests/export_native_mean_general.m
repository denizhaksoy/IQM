% Independent Octave mean(BB(:)) and Float32 DC fixtures, not GPU arithmetic.
1;
function words(file,a)
 assert(isa(a,'single')&&isreal(a)&&all(isfinite(a(:))));
 v=typecast(a(:),'uint32');f=fopen(file,'w');assert(f>=0);
 fprintf(f,'1 %d\n',numel(v));fprintf(f,'%u\n',v);fclose(f);
endfunction
args=argv();assert(numel(args)==1);out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
counts=[1 2 3 7 64 100 120 144 255 256 1024 4096];records={};
for n=counts
 folder=fullfile(out,sprintf('n%d',n));assert(mkdir(folder));
 manifest=fopen(fullfile(folder,'manifest.txt'),'w');assert(manifest>=0);
 for probe=1:12
  i=(0:n-1)';
  switch probe
   case 1,v=zeros(n,1,'single');
   case 2,v=-zeros(n,1,'single');
   case 3,v=ones(n,1,'single');
   case 4,v=single((-1).^i);
   case 5,v=single((-1).^i.*(1+mod(i*7919,65536)/65536).*2.^(-90+mod(i,17)));
   case 6,v=single((1+mod(i*3571,65536)/65536).*2.^(84+mod(i,17)));
   case 7,v=zeros(n,1,'single');v(1)=single(2^-90);
   case 8,v=single((-1).^i);v(end)=single(1+2^-23);
   case 9,v=repmat(single((2-2^-23)*2^100),n,1);
   case 10,v=repmat(single((2-2^-23)*2^100),n,1);v(end)=single(2^84);
   case 11,v=zeros(n,1,'single');v(1)=single((1+2^-23)*2^-90);if n>=2,v(2)=-single(2^-90);endif
   case 12,v=-repmat(single((2-2^-23)*2^100),n,1);v(end)=-single(2^84);
  endswitch
  m=mean(v(:));assert(isa(m,'single'));
  centered=v-m;tag=sprintf('probe%d',probe);
  input=fullfile(folder,[tag '_input.txt']);expected=fullfile(folder,[tag '_expected.txt']);
  words(input,v);words(expected,[m;centered]);
  fprintf(manifest,'%s "%s" "%s"\n',tag,strrep(make_absolute_filename(input),'\','/'),strrep(make_absolute_filename(expected),'\','/'));
 endfor
 fclose(manifest);records{end+1}=struct('count',n,'probes',12);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fprintf(f,'12 counts,12 independent native mean/DC fixtures each; single inputs and outputs.\n');fclose(f);
