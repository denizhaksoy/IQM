% Clean official quality from RGB16 PNGs, without persistent Float32 image dumps.
% Values and hashes must be validated against the original two-process scorer.
args=argv();assert(numel(args)==5,'official-root reference.png test.png fresh-output explicit-ppd');
official=canonicalize_file_name(args{1});out=args{4};ppd=str2double(args{5});
assert(isfinite(ppd)&&ppd>=26&&ppd<=256);assert(~exist(out,'dir'));assert(mkdir(out));diary(fullfile(out,'reference.log'));
assert(isequal(typecast(uint16(1),'uint8'),uint8([1 0])),'Little-endian host required for old export hash equivalence');
addpath(official);addpath(fullfile(official,'utils'));addpath(fullfile(official,'data'));addpath(fullfile(official,'examples'));
pkg load image;pkg load statistics;
assert(strcmp(hash('sha256',fileread(fullfile(official,'hdrvdp3.m'))),'0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a'));
images=cell(1,2);entries=struct([]);target=[];ambient=200*.005/pi;peak=2000;
tic;
for k=1:2
 raw=imread(args{k+1});assert(isa(raw,'uint16')&&size(raw,3)==3);
 if k==1,target=[size(raw,1),size(raw,2)];name='reference';else,name='test';end
 unit=im2single(raw);
 if k==2,unit=min(max(imresize(unit,target,'bicubic'),0),1);end
 luminance=min(pq2lin(unit)+ambient,peak+ambient);
 assert(isa(unit,'single')&&isa(luminance,'single')&&all(isfinite(luminance(:))));
 % Same byte sequence as fwrite(permute(luminance,[2,1,3]),'single','ieee-le').
 row_major=permute(luminance,[2,1,3]);bytes=typecast(row_major(:),'uint8');
 entry=struct('name',name,'source_sha256',hash('sha256',fileread(args{k+1})),...
  'linear_sha256',hash('sha256',char(bytes(:).')),'unit_class',class(unit),...
  'linear_class',class(luminance),'minimum',double(min(luminance(:))),...
  'maximum',double(max(luminance(:))),'values',numel(luminance));
 entries(k)=entry;images{k}=luminance;
 clear raw unit luminance row_major bytes entry;
end
preprocessing_seconds=toc;
fftw('threads',3);fftw('planner','estimate');fftw('swisdom','');fftw('dwisdom','');
options={'quiet',true,'use_gpu',false};
tic;res=hdrvdp3('quality',images{2},images{1},'rgb-bt.2020',ppd,options);elapsed=toc;
assert(isa(res.Q,'single')&&isa(res.Q_JOD,'single')&&isfinite(res.Q)&&isfinite(res.Q_JOD));
result=struct('scope','compact clean official PNG-to-score; not original paper cohort identity',...
 'octave_version',version(),'reference_version','3.0.7','backend','CPU','fftw_threads',3,...
 'fftw_planner','estimate','input_precision','single','ppd',ppd,'width',target(2),'height',target(1),...
 'preprocessing_seconds',preprocessing_seconds,'elapsed_seconds',elapsed,...
 'linear_layout','planar-row-major-little-endian-f32','persistent_linear_exports',false,...
 'Q_class',class(res.Q),'Q_JOD_class',class(res.Q_JOD),...
 'Q_text',sprintf('%.17g',res.Q),'Q_JOD_text',sprintf('%.17g',res.Q_JOD),...
 'Q_hex',num2hex(res.Q),'Q_JOD_hex',num2hex(res.Q_JOD),'inputs',entries);
f=fopen(fullfile(out,'score.json'),'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);
disp(jsonencode(result));diary off;
