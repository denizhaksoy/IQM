% Probe reference Minkowski expression on persisted D_p, not new metric cases.
args=argv();assert(numel(args)==4,'official-root captured-quality known-clean-score fresh-output');
official=args{1};stages=args{2};stock=jsondecode(fileread(args{3}));out=args{4};
assert(~exist(out,'dir'));assert(mkdir(out));
assert(strcmp(hash('sha256',fileread(fullfile(official,'hdrvdp3.m'))),'0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a'));
capture=jsondecode(fileread(fullfile(stages,'score.json')));
assert(strcmp(capture.Q_hex,stock.Q_hex)&&strcmp(capture.Q_JOD_hex,stock.Q_JOD_hex),'Capture did not preserve supplied clean baseline');
lines=strsplit(strtrim(fileread(fullfile(stages,'classes.jsonl'))),'\n');entries=cellfun(@jsondecode,lines,'UniformOutput',false);
errors=zeros(8,6,'single');records={};
for b=1:6
 tag=['quality_band',num2str(b),'_native'];target=['quality_error',num2str(b)];expected=[];
 for k=1:numel(entries),if strcmp(entries{k}.tag,target),expected=entries{k};end,end
 assert(~isempty(expected));file=fullfile(stages,[tag,'.f32']);
 f=fopen(file,'rb','ieee-le');assert(f>=0);raw=fread(f,Inf,'single=>single');fclose(f);
 hashes=cell(1,8);
 for offset=0:7
  storage=zeros(numel(raw)+offset,1,'single');storage(offset+1:end)=raw;
  X=storage(offset+1:end);assert(isequal(typecast(X,'uint32'),typecast(raw,'uint32')));
  % Exact expression from official local function minkowski_sum(X,p), p=0.8.
  p=0.8;value=sum(abs(X).^p/numel(X)).^(1/p);assert(isa(value,'single'));
  errors(offset+1,b)=value;hashes{offset+1}=num2hex(value);
 end
 records{b}=struct('band',b,'values',numel(raw),'input_sha256',hash('sha256',fileread(file)),...
  'captured_error_hex',expected.sample_hex,'offset_error_hex',{hashes});
end
scores=cell(1,8);
for offset=0:7
 total=single(0);for b=1:6,total=total+errors(offset+1,b)/6;end
 Q=6-log(exp(-4)+total);Q_JOD=10-0.5200*max(10-Q,0).^1.2812;
 scores{offset+1}=struct('offset',offset,'Q_err_hex',num2hex(total),'Q_hex',num2hex(Q),'Q_JOD_hex',num2hex(Q_JOD),...
  'matches_clean_baseline',strcmp(num2hex(Q),stock.Q_hex)&&strcmp(num2hex(Q_JOD),stock.Q_JOD_hex));
end
result=struct('scope','reference pooling on persisted later-run D_p values; does not establish initial failed-run tensors',...
 'octave_version',version(),'bands',{records},'scores',{scores});
f=fopen(fullfile(out,'report.json'),'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);
fprintf('Completed8offset pooling probes for6persisted bands\n');
