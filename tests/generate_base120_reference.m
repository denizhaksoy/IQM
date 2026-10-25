1;
function save_words(path,rows,columns,words)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows,columns);fprintf(f,'%u\n',words);fclose(f);
endfunction
function words=load_words(path)
 f=fopen(path,'r');assert(f>=0);shape=fscanf(f,'%d',2);words=uint32(fscanf(f,'%u'));fclose(f);assert(numel(words)==prod(shape));
endfunction
function e=reference(logs)
 assert(isa(logs,'single')&&numel(logs)==120&&all(isfinite(logs))&&all(logs>=-5&logs<=5));
 v=logs(logs!=0);exponents=floor(log2(abs(double(v))));assert(isempty(v)||max(exponents)-min(exponents)<=16);
 m=mean(logs(:));assert(isequal(typecast(m,'uint32'),typecast(single(sum(double(logs(:)))/120),'uint32')));
 adaptation=single(10).^m;assert(isa(adaptation,'single'));e=typecast([m;adaptation],'uint32');
endfunction
args=argv();out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));rng(1201026);
inputs=zeros(120*8192,1,'uint32');expected=zeros(2*8192,1,'uint32');
for k=0:8191
 logs=single((.03125+rand(120,1)*4.96875).*(2*(rand(120,1)>.5)-1));
 if k==0,logs(:)=single(0);elseif k==1,logs(:)=typecast(uint32(2147483648),'single');
 elseif k==2,logs(:)=single(5);elseif k==3,logs(:)=single(-5);elseif k==4,logs(1:2:end)=single(5);logs(2:2:end)=single(-5);endif
 inputs(k*120+(1:120))=typecast(logs,'uint32');expected(k*2+(1:2))=reference(logs);
endfor
save_words(fullfile(out,'synthetic_input.txt'),8192,120,inputs);save_words(fullfile(out,'synthetic_expected.txt'),8192,2,expected);
metadata=jsondecode(fileread(fullfile(source,'metadata.json')));assert(metadata.version==3.07);
inputs=[];expected=[];tags={};
for k=1:numel(metadata.records)
 record=metadata.records(k);a=load_words(fullfile(source,[record.tag '_input.txt']));v=typecast(a,'single');assert(all(v>0));
 logs=log10(v);assert(isa(logs,'single'));inputs=[inputs;typecast(logs,'uint32')];expected=[expected;reference(logs)];tags{end+1}=record.tag;
endfor
assert(numel(tags)==12);save_words(fullfile(out,'native_input.txt'),12,120,inputs);save_words(fullfile(out,'native_expected.txt'),12,2,expected);
f=fopen(fullfile(out,'manifest.txt'),'w');
fprintf(f,'synthetic "%s" "%s"\n',strrep(fullfile(out,'synthetic_input.txt'),'\','/'),strrep(fullfile(out,'synthetic_expected.txt'),'\','/'));
fprintf(f,'native "%s" "%s"\n',strrep(fullfile(out,'native_input.txt'),'\','/'),strrep(fullfile(out,'native_expected.txt'),'\','/'));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Bounded logmean/pow10 component only;not full adaptation/quality',...
 'version',3.07,'synthetic_cases',8192,'native_logmap_cases',12,'tags',{tags},'source_residual_directory',source,...
 'domain','120 finite single logs[-5,5],nonzero exponent span<=16','layout','120 log words -> mean,pow10 words',...
 'native_precision','native single mean and single(10).^mean;double accumulation assertion',...
 'log_source','native log10 of positive official standalone SPYR residuals;not full visual pathway L_adapt',...
 'normalization',120,'padding','none','DC','none;geometric-mean luminance only')));fclose(f);
fprintf('Base120 native reference:8192 synthetic+12 residual-derived logmaps,16408 output words\n');
