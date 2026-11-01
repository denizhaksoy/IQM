% Preserve the historical near-tie contract with fresh native traces.
1;
args=argv();assert(numel(args)==3);source=args{1};old=args{2};out=args{3};assert(!exist(out,'dir'));assert(mkdir(out));
a=load(fullfile(source,'h128_w128','trace.mat'));t=a.trace;
scores=single(dlmread(fullfile(old,'scores.txt')));
assert(isequal(typecast(single([a.baseline.Q,a.baseline.Q_JOD]),'uint32'),typecast(scores(:).','uint32')));
values=abs(t.base.spectrum);distinct=sort(unique(values(:)),'descend');assert(numel(distinct)>1);
gap=typecast(distinct(1),'uint32')-typecast(distinct(2),'uint32');
assert(gap==1&&distinct(1)==single(5018.5791015625)&&distinct(2)==single(5018.57861328125));
assert(t.base.index==33&&find(values(:)==max(values(:)),1)==33&&sum(values(:)==max(values(:)))==2);
f=fopen(fullfile(out,'ranking.txt'),'w');assert(f>=0);fprintf(f,'index_one_based maximum next_distinct gap_ULP ties\n%d %.17g %.17g %u %d\n',t.base.index,distinct(1),distinct(2),gap,sum(values(:)==max(values(:))));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fprintf(f,'Fresh native Q/Q_JOD and one-ULP near-tie reproduce historical pair;index33/two maxima preserved.\n');fclose(f);
