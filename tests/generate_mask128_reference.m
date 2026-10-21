% Native128x128 mutual masking and3x3 pool; no GPU files are read.
1;args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);
p=hdrvdp_parse_options('quality',{});assert(p.masking_norm==0&&p.masking_pool_size==3);
rng(12832026);n=128*128;
fi=fopen(fullfile(out,'input.txt'),'w');fo=fopen(fullfile(out,'expected.txt'),'w');
assert(fi>=0&&fo>=0);fprintf(fi,'16 %d\n',2*n);fprintf(fo,'16 %d\n',2*n);
normalizations=[];
for k=0:15
 a=single(randn(128,128)*2^(-12+2*k));b=single(randn(128,128)*2^(-12+2*k));
 if k==0,a(:)=0;b(:)=0;
 elseif k==1,a(:)=-3;b(:)=2;
 elseif k==2,a(:)=0;b(:)=0;a(1,1)=1;b(1,1)=-2;
 elseif k==3,a(:)=0;b(:)=0;a(127,3)=-1;b(127,3)=2;
 elseif k==4,a=reshape(single(mod(0:n-1,251)-125),128,128);b=-a*single(.75);endif
 normalization=2^mod(k,5);normalizations(end+1)=normalization;
 raw=min(abs(a/normalization),abs(b/normalization));pooled=conv2(raw,ones(3)/9,'same');
 assert(isa(raw,'single')&&isa(pooled,'single'));
 fprintf(fi,'%.17g\n',a');fprintf(fi,'%.17g\n',b');
 fprintf(fo,'%.17g\n',raw');fprintf(fo,'%.17g\n',pooled');
endfor
fclose(fi);fclose(fo);f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native128 mutual masking/raw+pool only','cases',16,'shape',[128,128],'precision','single','layout','percase row-major reference/test;expected raw/pooled','normalizations',normalizations,'pool','conv2(raw,ones(3)/9,same),zero outside','DC_processing','none')));fclose(f);
fprintf('Native128 masking:16 cases,524288 expected floats\n');
