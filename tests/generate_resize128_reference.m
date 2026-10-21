% Native Float32 imresize transitions needed by the128 pyramid frontier.
1;args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));pkg load image;rng(128322026);
pairs=[128,128;128,64;128,32;128,16;128,8;64,128;32,64;16,32;8,16;16,128];total=0;
for pair=1:rows(pairs)
 side=pairs(pair,1);target=pairs(pair,2);directory=fullfile(out,sprintf('from%d_to%d',side,target));assert(mkdir(directory));
 fi=fopen(fullfile(directory,'input.txt'),'w');fo=fopen(fullfile(directory,'expected.txt'),'w');fprintf(fi,'8 %d\n',side^2);fprintf(fo,'8 %d\n',target^2);
 [x,y]=meshgrid(0:side-1,0:side-1);
 for k=0:7
   a=single(randn(side)*2^(-10+3*k));
   if k==0,a(:)=0;elseif k==1,a(:)=single(2.5);
   elseif k==2,a(:)=0;a(1,1)=1;
   elseif k==3,a(:)=0;a(side,side-1)=-1;
   elseif k==4,a=single(mod(x+y,2));
   elseif k==5,a=single((3*x+7*y)/(10*side));endif
   b=imresize(a,[target,target]);assert(isa(b,'single'));fprintf(fi,'%.17g\n',a');fprintf(fo,'%.17g\n',b');total+=numel(b);
 endfor
 fclose(fi);fclose(fo);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Isolated Float32 resize frontier','pairs',pairs,'cases_per_pair',8,'compared_values',total,'precision','single','layout','row-major','interpolation','native imresize default bicubic/antialias/symmetric edges')));fclose(f);
fprintf('Native float resize128:80 transitions,%d expected floats\n',total);
