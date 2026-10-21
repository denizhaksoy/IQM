% Independent binary64 imresize references for128-square comparison masks.
1;args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));pkg load image;rng(128642026);
fi=fopen(fullfile(out,'input.txt'),'w');fprintf(fi,'8 16384\n');[x,y]=meshgrid(0:127,0:127);
for k=0:7
 a=double(rand(128)>.25);
 if k==0,a(:)=0;elseif k==1,a(:)=1;
 elseif k==2,a(:)=0;a(1,1)=1;
 elseif k==3,a(:)=0;a(128,127)=1;
 elseif k==4,a=double(mod(x+y,2)==0);
 elseif k==5,a=double(mod(3*x+7*y,19)<7);
 elseif k==6,a=double(rand(128)>.8);endif
 fprintf(fi,'%.17g\n',a');
 for side=[128,64,32,16,8]
   b=imresize(a,[side,side]);assert(isa(b,'double'));
   f=fopen(fullfile(out,sprintf('mask%d_to%d.txt',k,side)),'w');fprintf(f,'%d %d\n',side,side);fprintf(f,'%.17g\n',b');fclose(f);
 endfor
endfor
fclose(fi);f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native binary64 mask128 resize only','cases',8,'sizes',[128,64,32,16,8],'expected_double_values',174592,'precision','double','input','binary0/1','layout','row-major','interpolation','native image-package imresize default bicubic/antialias/symmetric edges')));fclose(f);
fprintf('Native mask128 resize:40 outputs,174592 binary64 values\n');
