1;
args=argv();assert(numel(args)==3);gpu=args{1};source=args{2};out=args{3};assert(!exist(out,'dir'));assert(mkdir(out));
loaded=load(fullfile(source,'h80_w96','trace.mat'));v=loaded.trace.visual_reference;x=v.pn.Y{1}(:);c=v.pn.jnd{1}(:);r=v.pn.jnd{2}(:);
expected=single([x;c;r;diff(c)./diff(x);diff(r)./diff(x);v.matrix([1,2,4]).']);assert(numel(expected)==10241);
f=fopen(fullfile(gpu,'h80_w96_reference_CPU_model_words.f32'),'rb','ieee-le');assert(f>=0);actual=fread(f,Inf,'*single');fclose(f);assert(numel(actual)==numel(expected));
indices=find(typecast(actual,'uint32')!=typecast(expected,'uint32'));counts=zeros(1,6);f=fopen(fullfile(out,'differences.tsv'),'w');fprintf(f,'word\tsection\tnode\tactual\texpected\tactual_bits\texpected_bits\n');
for i=indices(:).'
 if i<=2048,section=1;node=i;elseif i<=4096,section=2;node=i-2048;elseif i<=6144,section=3;node=i-4096;elseif i<=8191,section=4;node=i-6144;elseif i<=10238,section=5;node=i-8191;else,section=6;node=i-10238;endif
 counts(section)+=1;fprintf(f,'%d\t%d\t%d\t%.17g\t%.17g\t%u\t%u\n',i,section,node,actual(i),expected(i),typecast(actual(i),'uint32'),typecast(expected(i),'uint32'));
endfor
fclose(f);f=fopen(fullfile(out,'summary.json'),'w');fputs(f,jsonencode(struct('sections',{{'grid','cone_values','rod_values','cone_slopes','rod_slopes','matrix'}},'bit_mismatches',counts,'total',numel(indices),'scope','CPU generated JND model diagnostics; not an acceptance pass')));fclose(f);disp(counts);
