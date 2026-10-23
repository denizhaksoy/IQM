1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
rand('state',71501);
f=fopen(fullfile(output,'preparation.txt'),'w'); assert(f>=0);
for shape={[64,64],[13,19],[1,257]}
  h=shape{1}(1); w=shape{1}(2); n=h*w;
  for normalization=[1,2,3,4,8,16]
    r=single(10.^(-3+7*rand(n,1)));
    t=r.*single(1.001); bits=typecast(t,'uint32');
    offsets=mod((0:n-1)',3)-1;
    t=reshape(typecast(uint32(double(bits)+offsets),'single'),[],1);
    t(1:5:end)=r(1:5:end).*single(.999);
    difference=t/single(normalization)-r/single(normalization);
    mask=single(abs(t-r)./r>0.001);
    fprintf(f,'%d %d %.9g\n',h,w,normalization);
    fprintf(f,'%.9g\n',[r;t;difference;mask]);
  endfor
endfor
fclose(f);
f=fopen(fullfile(output,'dc.txt'),'w'); assert(f>=0);
for k=0:63
  base=-80+mod(k*13,161);
  values=single((1+rand(64,1)).*2.^(base+floor(17*rand(64,1))));
  values(1:2:end)=-values(1:2:end);
  if k==0, values(:)=0;
  elseif k==1, values(:)=single(1);
  elseif k==2, values(:)=single(1); values(1)=single(1)+eps('single'); endif
  expected=values-mean(values);
  fprintf(f,'64\n'); fprintf(f,'%.9g\n',[values;expected]);
endfor
fclose(f);
fprintf('Preparation reference:18 pairs and64 signed/constant DC cases\n');
