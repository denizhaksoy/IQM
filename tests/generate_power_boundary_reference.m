1;
% Float boundaries, subnormal bases/results, and densely sampled near1 inputs.
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rand('state',71499); count=65536;
bits=uint32(floor(double(hex2dec('49800000'))*rand(count,1)));
bits(1:32768)=uint32(double(hex2dec('3f800000'))+(-16384:16383)');
bits(32769:32776)=uint32([1,2,hex2dec('007fffff'),hex2dec('00800000'), ...
  hex2dec('00800001'),hex2dec('00008000'),hex2dec('3f700000'),hex2dec('3f880000')]);
x=reshape(typecast(bits,'single'),[],1);
exponents=single([10^.108934275615,10^.3424,2*10^.3424,10^.34/10^.3424,.8,1.25,1.2812,.41,2]);
y=exponents(1+mod((0:count-1)',numel(exponents))); y=y(:);
expected=x.^y; assert(all(isfinite(expected)));
for entry={'input128.txt','forward128.txt'}
  data=[x,y]; if strcmp(entry{1},'forward128.txt'), data=[expected,expected]; endif
  fid=fopen(fullfile(output,entry{1}),'w'); assert(fid>=0);
  fprintf(fid,'512 256\n'); fprintf(fid,'%.9g\n',data'); fclose(fid);
endfor
fprintf('Native boundary power reference: %d pairs; 32768 near1 bases\n',count);
