% Reuse bounded official SPYR inputs; independently compute native FFT contract.
1;
function words=load_words(path)
 f=fopen(path,'r');assert(f>=0);shape=fscanf(f,'%d',2);words=uint32(fscanf(f,'%u'));fclose(f);assert(numel(words)==prod(shape));
endfunction
function save_words(path,rows,columns,words)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows,columns);fprintf(f,'%u\n',words);fclose(f);
endfunction
args=argv();out=args{1};mean_source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
metadata=jsondecode(fileread(fullfile(mean_source,'metadata.json')));assert(metadata.version==3.07);
fftw('threads',3);fftw('planner','estimate');manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);
for orientation=1:2
 h=10;w=12;if orientation==2,h=12;w=10;endif
 input=[];means=[];packed=[];expected=[];tags={};
 for k=1:numel(metadata.records)
  record=metadata.records(k);if !isequal(reshape(record.residual_shape,1,[]),[h,w]),continue;endif
  a=load_words(fullfile(mean_source,[record.tag '_input.txt']));b=load_words(fullfile(mean_source,[record.tag '_expected.txt']));
  input=[input;a];means=[means;b];centered=reshape(typecast(b(2:end),'single'),[w,h]).';
  v=centered(:);z=zeros(240,1,'uint32');z(1:2:end)=typecast(v,'uint32');packed=[packed;z];
  fftw('swisdom','');spectrum=fft2(centered);magnitude=abs(spectrum);magnitude(1,1)=0;
  [maximum,index]=max(magnitude(:));assert(isfinite(maximum));
  row=reshape(spectrum.',[],1);e=zeros(361,1,'uint32');
  e(1:2:240)=typecast(real(row),'uint32');e(2:2:240)=typecast(imag(row),'uint32');
  e(241:360)=typecast(reshape(magnitude.',[],1),'uint32');e(361)=typecast(single(index-1),'uint32');
  expected=[expected;e];tags{end+1}=record.tag;
 endfor
 count=numel(tags);assert(count==6);directory=fullfile(out,sprintf('h%d_w%d',h,w));assert(mkdir(directory));
 names={'input','mean','pack','expected'};values={input,means,packed,expected};widths=[120,121,240,361];paths={};
 for i=1:4
  path=strrep(fullfile(directory,[names{i} '.txt']),'\','/');save_words(path,count,widths(i),values{i});paths{i}=path;
 endfor
 fprintf(manifest,'h%d_w%d %d %d "%s" "%s" "%s" "%s"\n',h,w,h,w,paths{:});
 f=fopen(fullfile(directory,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'tags',{tags},...
  'scope','Bounded mean/DC -> packing -> residualFFT/magnitude/firstmax;not full quality',...
  'input_layout','row-major real120;mean+centered121;col-major complex240;row-major FFT240+magnitude120+index1',...
  'normalization','mean/120 only;forward unnormalized','padding','none','threads',3,'planner','estimate/fresh single wisdom',...
  'DC','subtract native single mean;zero DC magnitude only;raw spectrum unchanged')));fclose(f);
endfor
fclose(manifest);fprintf('Native GPU-pipeline reference:12 cases,1452 mean words,2880 packed words,4332 final words\n');
