1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rng(20261017,'twister'); fid=fopen(fullfile(output,'pool.txt'),'w'); assert(fid>=0); count=0;
for n=[64,32,16,8]
  [x,y]=meshgrid(0:n-1,0:n-1);
  for pattern=1:6
    if pattern==1
      reference=single(10.^(40*rand(n)-20));
    elseif pattern==2
      reference=zeros(n,'single'); reference(1,end)=1e15; reference(end,1)=1e-15; reference(3,5)=17;
    elseif pattern==3
      reference=single((-1).^(x+y)).*(1+single(mod(3*x+5*y,17))*eps('single'));
    elseif pattern==4
      reference=zeros(n,'single');
    elseif pattern==5
      reference=ones(n,'single')*10000;
    else
      reference=single((rand(n)-0.5)*100);
    endif
    test=reference.*single(0.8+0.4*rand(n)); normalization=2^mod(pattern-1,5);
    raw=min(abs(reference/normalization),abs(test/normalization));
    pooled=conv2(raw,ones(3)/9,'same');
    fprintf(fid,'%d %d %g\n',n,n,normalization);
    for matrix={reference,test,raw,pooled}
      fprintf(fid,'%.9g ',matrix{1}'); fprintf(fid,'\n');
    endfor
    count+=1;
  endfor
endfor
fclose(fid); fprintf('Native Octave conv2 pool cases=%d\n',count);
