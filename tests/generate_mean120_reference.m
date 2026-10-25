% Official SPYR residual mean/DC contract, bounded120-sample component fixtures.
1;
function write_words(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'1 %d\n',numel(a));
 fprintf(f,'%u\n',typecast(a(:),'uint32'));fclose(f);
endfunction
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);
pkg load image;pkg load statistics;
fftw('threads',3);fftw('planner','estimate');
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);records={};
for shape=1:2
 h=80;w=96;if shape==2,h=96;w=80;endif
 [x,y]=meshgrid(0:w-1,0:h-1);
 for pattern=1:3
  a=single(10)+single(.01)*single(mod(3*x+7*y+x.*y,101)-50);
  if pattern==2,a(:)=single(10);a(3,5)=single(17);endif
  if pattern==3,a=single(10.^(-2+5*(3*x/(w-1)+2*y/(h-1))/5));endif
  for label=0:1
   input=a;if label,input=input*single(1.05);endif
   pyramid=hdrvdp_spyr('sp0Filters');pyramid=pyramid.decompose(input,30);
   residual=pyramid.get_band(pyramid.band_count(),1);assert(numel(residual)==120&&isa(residual,'single'));
   v=reshape(residual.',[],1);nz=abs(v(v!=0));exponents=floor(log2(double(nz)));
   assert(isempty(exponents)||(min(exponents)>=-90&&max(exponents)<=100&&max(exponents)-min(exponents)<=16));
   mean_native=mean(residual(:));assert(isa(mean_native,'single'));
   assert(isequal(typecast(mean_native,'uint32'),typecast(single(sum(double(v))/120),'uint32')));
   centered=v-mean_native;expected=[mean_native;centered];
   tag=sprintf('h%d_w%d_p%d_l%d',h,w,pattern,label);
   in_path=strrep(fullfile(out,[tag '_input.txt']),'\','/');expected_path=strrep(fullfile(out,[tag '_expected.txt']),'\','/');
   write_words(in_path,v);write_words(expected_path,expected);
   fprintf(manifest,'%s "%s" "%s"\n',tag,in_path,expected_path);
   records{end+1}=struct('tag',tag,'shape',[h,w],'residual_shape',size(residual),'minimum_exponent',min(exponents),...
    'maximum_exponent',max(exponents),'mean',mean_native);
  endfor
 endfor
endfor
fclose(manifest);f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',hdrvdp_version(),...
 'scope','Official SPYR residual mean/DC only;no complete quality or base pow10',...
 'ppd',30,'precision','single inputs;double sum/division;single mean and subtraction',...
 'layout','row-major120 words;expected mean then120 centered words',...
 'patterns','asymmetric;constant10 plus impulse7;bounded HDR ramp1e-2..1e3',...
 'normalization',120,'padding','official SPYR component only;no MTF/LA in this unit',...
 'records',{records})));fclose(f);fprintf('Native mean120/DC:12 fixtures,1452 expected words\n');
