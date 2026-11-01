% Run the unmodified official metric at original image size. This harness is
% generic; a UPIQ manifest must come from verified dataset metadata, not guesses.
1;
function image=load_case_image(path,precision,decoded,channels)
  [~,~,ext]=fileparts(path);
  if strcmpi(ext,'.exr')
    assert(~isempty(decoded),'EXR must be prepared with prepare_exr_inputs.cjs');
    meta=jsondecode(fileread(fullfile(decoded,'image.json')));
    assert(strcmp(meta.channels,channels)&&strcmp(meta.source_sha256,hash('sha256',fileread(path))));
    assert(strcmp(meta.layout,'planar-row-major-little-endian-f32')&&~meta.normalization&&~meta.color_transform&&~meta.resized);
    assert(isequal(meta.data_window,meta.display_window),'Differing EXR display/data windows need an explicit full-image policy');
    binary=fullfile(decoded,'image.f32');assert(strcmp(meta.decoded_sha256,hash('sha256',fileread(binary))));
    f=fopen(binary,'rb','ieee-le');assert(f>=0);raw=fread(f,Inf,'single=>single');fclose(f);
    assert(numel(raw)==meta.width*meta.height*3);image=permute(reshape(raw,[meta.width,meta.height,3]),[2,1,3]);
  elseif strcmpi(ext,'.pfm')
    f=fopen(path,'rb','ieee-le');assert(f>=0);
    unwind_protect
      magic=strtrim(fgetl(f));assert(strcmp(magic,'PF'));
      dims=sscanf(fgetl(f),'%d');assert(numel(dims)==2&&all(dims>0));w=dims(1);h=dims(2);
      scale=str2double(fgetl(f));assert(isfinite(scale)&&scale~=0);
      if scale>0,endian='ieee-be';else,endian='ieee-le';end
      [raw,count]=fread(f,3*w*h,'single=>single',0,endian);assert(count==3*w*h&&isempty(fread(f,1,'uint8')));
      image=flipud(permute(reshape(raw,[3,w,h]),[3,2,1]));
      image=image*single(abs(scale));
    unwind_protect_cleanup
      fclose(f);
    end_unwind_protect
  elseif any(strcmpi(ext,{'.png','.bmp','.tif','.tiff'}))
    raw=imread(path);assert(isinteger(raw),'Image reader must return original integer samples');
    image=double(raw)/double(intmax(class(raw)));
  else
    error('Unverified decoder for %s; EXR ingestion must preserve actual floating samples',ext);
  end
  assert(any(strcmp(precision,{'single','double'})));image=cast(image,precision);
  assert(ndims(image)==3&&size(image,3)==3&&all(isfinite(image(:)))&&all(image(:)>=0));
end
args=argv();assert(numel(args)==3,'Usage: reference-root manifest.json fresh-output');
official=canonicalize_file_name(args{1});manifest=canonicalize_file_name(args{2});out=args{3};
assert(~isempty(official)&&~isempty(manifest)&&~exist(out,'dir'));assert(mkdir(out));
addpath(official);addpath(fullfile(official,'utils'));addpath(fullfile(official,'data'));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);assert(strcmp(canonicalize_file_name(which('hdrvdp3')),canonicalize_file_name(fullfile(official,'hdrvdp3.m'))));
spec=jsondecode(fileread(manifest));assert(strcmp(spec.format,'IQM_REFERENCE_MANIFEST_1'));
assert(isfield(spec,'dataset_status')&&isfield(spec,'cases'));
fftw('threads',3);fftw('planner','estimate');
results=struct('dataset_status',spec.dataset_status,'reference_version','3.0.7','octave_version',version(),'fftw_threads',3,'fftw_planner','estimate','reference_source_sha256',hash('sha256',fileread(fullfile(official,'hdrvdp3.m'))),'cases',{{}});
base=fileparts(manifest);
for i=1:numel(spec.cases)
  c=spec.cases(i);assert(~isempty(regexp(c.id,'^[A-Za-z0-9_-]+$','once')));
  assert(c.ppd>0&&isfinite(c.ppd)&&isfinite(c.observer_age));
  assert(any(strcmp(c.encoding,{'sRGB-display','rgb-native','rgb-bt.709','rgb-bt.2020','XYZ'})));
  if is_absolute_filename(c.reference),ref=c.reference;else,ref=fullfile(base,c.reference);end
  if is_absolute_filename(c.test),test=c.test;else,test=fullfile(base,c.test);end
  ref=canonicalize_file_name(ref);test=canonicalize_file_name(test);assert(~isempty(ref)&&~isempty(test));
  refDecoded='';testDecoded='';channels='';
  if isfield(c,'reference_decoded'),refDecoded=c.reference_decoded;end
  if isfield(c,'test_decoded'),testDecoded=c.test_decoded;end
  if isfield(c,'source_channels'),channels=c.source_channels;end
  reference=load_case_image(ref,c.precision,refDecoded,channels);distorted=load_case_image(test,c.precision,testDecoded,channels);
  assert(isequal(size(reference),size(distorted)),'Original image dimensions differ');
  % No resizing, cropping, tone mapping, display-unit normalization or guessed ppd.
  options={'use_gpu',false,'quiet',true,'age',c.observer_age,'rgb_display',c.rgb_display};
  fftw('swisdom','');fftw('dwisdom','');tic;
  log=evalc("score=hdrvdp3('quality',distorted,reference,c.encoding,c.ppd,options);");elapsed=toc;
  f=fopen(fullfile(out,[c.id,'.log']),'wt');fprintf(f,'%s',log);fclose(f);
  assert(isscalar(score.Q)&&isscalar(score.Q_JOD)&&isfinite(score.Q)&&isfinite(score.Q_JOD));
  f=fopen(fullfile(out,[c.id,'_quality.txt']),'wt');fprintf(f,'%.17g %.17g\n',score.Q,score.Q_JOD);fclose(f);
  record=struct('id',c.id,'reference_sha256',hash('sha256',fileread(ref)),'test_sha256',hash('sha256',fileread(test)),'height',size(reference,1),'width',size(reference,2),'precision',c.precision,'encoding',c.encoding,'ppd',c.ppd,'observer_age',c.observer_age,'rgb_display',c.rgb_display,'Q_text',sprintf('%.17g',score.Q),'Q_JOD_text',sprintf('%.17g',score.Q_JOD),'Q_hex',num2hex(score.Q),'Q_JOD_hex',num2hex(score.Q_JOD),'seconds',elapsed);
  results.cases{end+1}=record;
  f=fopen(fullfile(out,'results.json'),'wt');fputs(f,jsonencode(results));fclose(f);
  save('-mat7-binary',fullfile(out,[c.id,'_result.mat']),'score');
  fprintf('%s %dx%d Q=%.17g Q_JOD=%.17g\n',c.id,size(reference,2),size(reference,1),score.Q,score.Q_JOD);
end
f=fopen(fullfile(out,'COMPLETE.txt'),'wt');fprintf(f,'Reference execution complete;dataset_status=%s;not implementation parity acceptance\n',spec.dataset_status);fclose(f);
