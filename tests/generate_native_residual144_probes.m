% Separate FFT representation probes, not full-pathway image acceptance.
1;
args=argv();assert(numel(args)==1);out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
fftw('threads',3);fftw('planner','estimate');records={};
labels={'zero','constant','origin_impulse','nyquist','shifted_impulse'};
for k=1:numel(labels)
 input=single(zeros(12));
 if k==2,input(:)=single(.125);elseif k==3,input(1,1)=single(.25);
 elseif k==4,[x,y]=meshgrid(0:11);input=single((-1).^(x+y)*.125);
 elseif k==5,input(3,5)=single(.75);endif
 fftw('swisdom','');expected=fft2(input);folder=fullfile(out,labels{k});assert(mkdir(folder));
 for entry={'input','expected'}
  name=entry{1};if strcmp(name,'input'),a=input;else,a=expected;endif
  f=fopen(fullfile(folder,[name '.txt']),'w');assert(f>=0);fprintf(f,'12 24\n');
  ar=real(a);ai=imag(a);
  for y=1:12,for x=1:12,fprintf(f,'%.17g\n%.17g\n',ar(y,x),ai(y,x));endfor;endfor;fclose(f);
 endfor
 records{end+1}=struct('case',labels{k},'type',typeinfo(expected),'entirely_real',isreal(expected));
endfor
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
