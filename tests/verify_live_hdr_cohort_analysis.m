% Independent statistics verification, not numerical metric tolerance acceptance.
args=argv();assert(numel(args)==2,'analysis.json fresh-verification.json');
source=args{1};target=args{2};assert(~exist(target,'file'));
raw=fileread(source);report=jsondecode(raw);rows=report.rows;n=numel(rows);
assert(n>=3&&report.videos==n&&~report.fitted_mapping&&~report.retraining);
assert(strcmp(report.target,'bright_mos')&&strcmp(report.prediction,'native video mean Q_JOD'));
assert(isempty(report.RMSE),'No undocumented JOD-to-MOS mapping permitted');
names={rows.video};assert(numel(unique(names))==n);
y=[rows.mos]';official=[rows.official]';cpp=[rows.cpp]';
assert(all(isfinite(y))&&all(isfinite(official))&&all(isfinite(cpp)));
assert(all(official==cpp)&&std(y)>0&&std(official)>0);
expected=struct('raw_PLCC',corr(official,y),'SROCC',spearman(official,y));
tolerance=1e-14;keys={'raw_PLCC','SROCC'};
for k=1:numel(keys)
 key=keys{k};assert(abs(report.official.(key)-expected.(key))<=tolerance);
 assert(abs(report.cpp.(key)-expected.(key))<=tolerance);
end
result=struct('scope','independent verification of supplied analysis; does not establish original-author cohort identity',...
 'passed',true,'videos',n,'analysis_sha256',hash('sha256',raw),...
 'octave_version',version(),'maximum_absolute_statistics_tolerance',tolerance,...
 'official',expected,'cpp',expected,'author_cohort_verified',report.author_cohort_verified);
f=fopen(target,'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);disp(jsonencode(result));
