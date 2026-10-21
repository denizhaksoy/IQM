% Independent native DC/mean/power10 references for64 or256 samples.
1;args=argv();out=args{1};n=str2double(args{2});assert(any(n==[64,256]));
assert(!exist(out,'dir'));assert(mkdir(out));rng(2561920+n);
fi=fopen(fullfile(out,'input.txt'),'w');fo=fopen(fullfile(out,'expected.txt'),'w');
assert(fi>=0&&fo>=0);fprintf(fi,'512 %d\n',2*n);fprintf(fo,'512 %d\n',n+3);
ff=-1;if n==256
 fftw('threads',3);fftw('swisdom','');ff=fopen(fullfile(out,'expected_fft16.txt'),'w');assert(ff>=0);fprintf(ff,'512 769\n');
endif
for k=0:511
 logs=single((rand(n,1)-.5)*10);
 if k<129,logs(:)=single(-5+10*k/128);
 elseif k<258
   value=single(-5+10*(k-129)/128);bits=typecast(value,'uint32');
   if abs(value)==5,next=typecast(bits-uint32(1),'single');
   elseif value!=0,next=typecast(bits+uint32(1),'single');else next=single(2^-20);endif
   logs(:)=value;logs(1:2:end)=next;
 elseif k<322
   logs=single([ones(n/2,1);-ones(n/2,1)]);logs(1)=single(1+2^(-16+mod(k,17)));
 elseif k<386,logs=single((rand(n,1)-.5)*2^-12);endif
 nz=abs(logs(logs!=0));if !isempty(nz)
   e=floor(log2(double(nz)));logs(abs(logs)<single(2^(max(e)-16)))=0;
 endif
 residual=single((rand(n,1)-.5)*2^(-30+mod(k,61)));
 if k==0,residual(:)=0;elseif k==1,residual(:)=single(10);
 elseif k==2,residual=single([ones(n/2,1);-ones(n/2,1)]);
 elseif k==3,residual(:)=single(2^-90);residual(1)=single(2^-74);
 elseif k==4,residual(:)=single(2^84);residual(1)=single(2^100);endif
 nz=abs(residual(residual!=0));if !isempty(nz)
   e=floor(log2(double(nz)));residual(abs(residual)<single(2^(max(e)-16)))=0;
 endif
 dcmean=mean(residual);centered=residual-dcmean;m=mean(logs);power=10.^m;
 assert(isa(dcmean,'single')&&isa(centered,'single')&&isa(m,'single')&&isa(power,'single'));
 fprintf(fi,'%.17g\n',[residual;logs]);fprintf(fo,'%.17g\n',[centered;dcmean;m;power]);
 if ff>=0
   spectrum=fft2(reshape(centered,16,16).');row=reshape(spectrum.',[],1);
   magnitudes=abs(spectrum);magnitudes(1,1)=0;index=find(magnitudes(:)==max(magnitudes(:)),1)-1;
   fprintf(ff,'%.17g\n',[real(row)';imag(row)']);fprintf(ff,'%.17g\n',magnitudes');fprintf(ff,'%d\n',index);
 endif
endfor
fclose(fi);fclose(fo);
if ff>=0,fclose(ff);endif
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Independent native DC/base adaptation','samples',n,'cases',512,'precision','single','sum','native Octave mean(single)','residual_exponent_span_max',16,'residual_exponents',[-90,100],'log_domain',[-5,5],'log_exponent_span_max',16,'input_layout','per case residual vector thenlog vector','output_layout','centered vector,DC mean,log mean,power10')));fclose(f);
fprintf('Native reductions:512 cases,N=%d,expected=%d scalars\n',n,512*(n+3));
