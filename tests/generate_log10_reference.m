1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rand('state',71500); count=65536;
x=single(10.^(-5+10*rand(count,1)));
bits=uint32(double(hex2dec('3f800000'))+(-16384:16383)');
x(1:32768)=reshape(typecast(bits,'single'),[],1);
expected=log10(x);
for entry={'input128.txt','forward128.txt'}
  data=[x,x]; if strcmp(entry{1},'forward128.txt'), data=[expected,expected]; endif
  fid=fopen(fullfile(output,entry{1}),'w'); assert(fid>=0);
  fprintf(fid,'512 256\n'); fprintf(fid,'%.9g\n',data'); fclose(fid);
endfor
