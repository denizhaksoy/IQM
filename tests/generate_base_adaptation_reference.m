1;
% Independent native single mean/power10 fixture; no GPU results are read.
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rand('state',71803);
f=fopen(fullfile(output,'base_adaptation.txt'),'w'); assert(f>=0);
for k=0:511
  x=single((rand(8,8)-.5)*10);
  if k<129
    x(:)=single(-5+10*k/128);
  elseif k<258
    % Adjacent single values around positive and negative means.
    value=single(-5+10*(k-129)/128);
    bits=typecast(value,'uint32');
    if abs(value)==5, next=typecast(bits-uint32(1),'single');
    elseif value!=0, next=typecast(bits+uint32(1),'single');
    else next=single(2^-20); endif
    x(:)=value; x(1:2:end)=next;
  elseif k<322
    % Exact signed cancellation, then a representable small perturbation.
    x=reshape(single([ones(1,32),-ones(1,32)]),8,8);
    x(1)=single(1+2^(-16+mod(k,17)));
  elseif k<386
    x=single((rand(8,8)-.5)*2^-12);
  endif
  % Enforce the same declared exponent-span domain without consulting GPU.
  nonzero=abs(x(x!=0));
  if !isempty(nonzero)
    e=floor(log2(double(nonzero)));
    if max(e)-min(e)>16, x(abs(x)<single(2^(max(e)-16)))=0; endif
  endif
  m=mean(x(:)); y=10.^m; assert(isa(m,'single')&&isa(y,'single'));
  fprintf(f,'64\n'); fprintf(f,'%.9g\n',x'); fprintf(f,'%.9g %.9g\n',m,y);
endfor
fclose(f);
fprintf('Native base adaptation:512 cases,1024 expected scalar values\n');
