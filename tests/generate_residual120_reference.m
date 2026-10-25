% Independent official small residual FFTs, not complete quality acceptance.
1;
function words=load_words(path)
 f=fopen(path,'r');assert(f>=0);shape=fscanf(f,'%d',2);words=uint32(fscanf(f,'%u'));fclose(f);assert(numel(words)==prod(shape));
endfunction
args=argv();out=args{1};mean_source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
fftw('threads',3);fftw('planner','estimate');rng(1202026);
metadata=jsondecode(fileread(fullfile(mean_source,'metadata.json')));records=metadata.records;
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);
extended_manifest=fopen(fullfile(out,'manifest_extended.txt'),'w');assert(extended_manifest>=0);
for orientation=1:2
 h=10;w=12;if orientation==2,h=12;w=10;endif
 directory=fullfile(out,sprintf('h%d_w%d',h,w));assert(mkdir(directory));
 inputs={};tags={};[x,y]=meshgrid(0:w-1,0:h-1);
 for k=0:31
  a=single((.5+rand(h,w)).*(2*(rand(h,w)>.5)-1)*2^(-10+mod(k,21)));
  if k==0,a(:)=0;elseif k==1,a(:)=single(1);
  elseif k==2,a(:)=0;a(3,5)=1;
  elseif k==3,a=single(cos(2*pi*x/w));
  elseif k==4,a=single(cos(2*pi*y/h));
  elseif k==5,a=single(cos(2*pi*x/w)+cos(4*pi*y/h));
  elseif k==6,a(:)=typecast(uint32(2147483648),'single');endif
  a=a-mean(a(:));inputs{end+1}=a;tags{end+1}=sprintf('synthetic%d',k);
 endfor
 for k=1:numel(records)
  record=records(k);if !isequal(reshape(record.residual_shape,1,[]),[h,w]),continue;endif
  words=load_words(fullfile(mean_source,[record.tag '_expected.txt']));
  centered=typecast(words(2:end),'single');a=reshape(centered,[w,h]).';
  inputs{end+1}=a;tags{end+1}=record.tag;
 endfor
 count=numel(inputs);assert(count==38);
 fi=fopen(fullfile(directory,'input.txt'),'w');fo=fopen(fullfile(directory,'expected.txt'),'w');
 fe=fopen(fullfile(directory,'expected_extended.txt'),'w');fprintf(fe,'%d 361\n',count);
 fprintf(fi,'%d %d\n',count*h,2*w);fprintf(fo,'%d %d\n',count*h,2*w);
 for k=1:count
  a=inputs{k};assert(isa(a,'single')&&isequal(size(a),[h,w]));
  fftw('swisdom','');spectrum=fft2(a);assert(isa(spectrum,'single'));
  fprintf(fi,'%.17g\n',[a(:).';zeros(1,120,'single')]);
  v=reshape(spectrum.',[],1);fprintf(fo,'%.17g\n',[real(v).';imag(v).']);
  magnitudes=abs(spectrum);assert(isa(magnitudes,'single'));magnitudes(1,1)=single(0);
  [maximum,index]=max(magnitudes(:));assert(isfinite(maximum));
  fprintf(fe,'%.17g\n',[real(v).';imag(v).']);fprintf(fe,'%.17g\n',magnitudes.');fprintf(fe,'%d\n',index-1);
 endfor
 fclose(fi);fclose(fo);
 fclose(fe);
 f=fopen(fullfile(directory,'metadata.json'),'w');fputs(f,jsonencode(struct('h',h,'w',w,'count',count,'tags',{tags},...
  'input_layout','per matrix column-major interleaved complex real with imag0','expected_layout','per matrix row-major interleaved complex',...
  'precision','single','threads',3,'planner','estimate;fresh single wisdom before each fft2',...
  'normalization','none','DC','native mean subtraction before input export;raw spectrum DC preserved',...
  'padding','none;no MTF/LA filter','source_mean_directory',mean_source)));fclose(f);
 fprintf(manifest,'h%d_w%d %d %d "%s" "%s"\n',h,w,h,w,strrep(fullfile(directory,'input.txt'),'\','/'),strrep(fullfile(directory,'expected.txt'),'\','/'));
 fprintf(extended_manifest,'h%d_w%d %d %d "%s" "%s"\n',h,w,h,w,strrep(fullfile(directory,'input.txt'),'\','/'),strrep(fullfile(directory,'expected_extended.txt'),'\','/'));
 fprintf('Residual%dx%d:38 cases,9120 complex components\n',h,w);
endfor
fclose(manifest);
fclose(extended_manifest);
