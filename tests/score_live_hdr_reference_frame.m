% Score an already verified original-size Single pair with clean v3.0.7.
args=argv();assert(numel(args)==3,'official-root preprocessing-root fresh-output');
official=canonicalize_file_name(args{1});base=canonicalize_file_name(args{2});out=args{3};
assert(~exist(out,'dir'));assert(mkdir(out));diary(fullfile(out,'reference.log'));
addpath(official);addpath(fullfile(official,'utils'));addpath(fullfile(official,'data'));
pkg load image;pkg load statistics;
assert(strcmp(hash('sha256',fileread(fullfile(official,'hdrvdp3.m'))),'0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a'));
meta=jsondecode(fileread(fullfile(base,'preprocessing.json')));
images=cell(1,2);
for k=1:2
 entry=meta.inputs(k);file=fullfile(base,[entry.name,'.f32']);
 assert(strcmp(hash('sha256',fileread(file)),entry.linear_sha256));
 f=fopen(file,'rb','ieee-le');assert(f>=0);raw=fread(f,Inf,'single=>single');fclose(f);
 assert(numel(raw)==meta.width*meta.height*3);
 images{k}=permute(reshape(raw,[meta.width,meta.height,3]),[2,1,3]);
end
fftw('threads',3);fftw('planner','estimate');fftw('swisdom','');fftw('dwisdom','');
options={'quiet',true,'use_gpu',false};
tic;res=hdrvdp3('quality',images{2},images{1},'rgb-bt.2020',meta.ppd,options);elapsed=toc;
assert(isfinite(res.Q)&&isfinite(res.Q_JOD));
result=struct('scope','one original-size local RGB48-decoded frame; not video or dataset equivalence',...
 'octave_version',version(),'reference_version','3.0.7','backend','CPU','fftw_threads',3,...
 'fftw_planner','estimate','input_precision','single','ppd',meta.ppd,'elapsed_seconds',elapsed,...
 'Q_class',class(res.Q),'Q_JOD_class',class(res.Q_JOD),...
 'Q_text',sprintf('%.17g',res.Q),'Q_JOD_text',sprintf('%.17g',res.Q_JOD),...
 'Q_hex',num2hex(res.Q),'Q_JOD_hex',num2hex(res.Q_JOD),'inputs',meta.inputs);
f=fopen(fullfile(out,'score.json'),'wt');fprintf(f,'%s\n',jsonencode(result));fclose(f);
disp(jsonencode(result));diary off;
