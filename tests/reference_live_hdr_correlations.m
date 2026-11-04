args=argv();assert(numel(args)==1&&!exist(args{1},'file'));
x=[1;2;2;4;5;5];y=[6;3;5;2;2;1];
result=struct('pearson',corr(x,y),'spearman',spearman(x,y),'octave_version',version());
f=fopen(args{1},'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);disp(jsonencode(result));
