% Saved-band diagnostic only; not a reproduction of the initial failed run.
args=argv();assert(numel(args)==3,'official-root captured-stages fresh-output');
official=canonicalize_file_name(args{1});stages=canonicalize_file_name(args{2});out=args{3};
assert(~exist(out,'dir'));assert(mkdir(out));addpath(official);addpath(fullfile(official,'utils'));
lines=strsplit(strtrim(fileread(fullfile(stages,'classes.jsonl'))),'\n');entry=[];
for k=1:numel(lines),e=jsondecode(lines{k});if strcmp(e.tag,'reference_band6'),entry=e;end,end
assert(~isempty(entry)&&strcmp(entry.class,'single'));sz=entry.shape(:)';
images=cell(1,2);names={'reference','test'};
for k=1:2
 f=fopen(fullfile(stages,[names{k},'_band6.f32']),'rb','ieee-le');assert(f>=0);
 v=fread(f,Inf,'single=>single');fclose(f);assert(numel(v)==prod(sz));images{k}=reshape(v,sz);
end
band_diff=images{2}-images{1};ppd=62.729130063099447;
base_band_frequency=2^-5*ppd/2;rho=create_cycdeg_image(sz,base_band_frequency*4);
fftw('threads',3);fftw('planner','estimate');records=cell(1,32);
for offset=0:31
 storage=zeros(numel(band_diff)+offset,1,'single');storage(offset+1:end)=band_diff(:);
 X=reshape(storage(offset+1:end),sz);assert(isequal(typecast(X(:),'uint32'),typecast(band_diff(:),'uint32')));
 fftw('swisdom','');fftw('dwisdom','');F=fft2(X);F(1)=0;mag=abs(F(:));peak=max(mag);index=find(mag==peak,1);
 file=fullfile(out,sprintf('spectrum_offset%02d.f32',offset));f=fopen(file,'wb','ieee-le');assert(f>=0);fwrite(f,[real(F(:))';imag(F(:))'],'single');fclose(f);
 % Classify conjugate bins with physically equal frequency, without changing
 % the actual selected index/frequency or numerical metric acceptance.
 other=abs(rho(:)-rho(index))>1e-12;second=max(mag(other));
 records{offset+1}=struct('offset',offset,'selected_column_major_index_one_based',index,...
  'frequency',rho(index),'frequency_hex',num2hex(rho(index)),...
  'peak_hex',num2hex(peak),'relative_gap_other_frequency',double((peak-second)/peak),...
  'spectrum_sha256',hash('sha256',fileread(file)));
end
result=struct('scope','FFT of persisted bands from later matching capture; not initial failed-run intermediate evidence',...
 'octave_version',version(),'fftw_threads',3,'planner','estimate','shape',sz,...
 'physical_frequency_classification_tolerance',1e-12,'runs',{records});
f=fopen(fullfile(out,'report.json'),'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);
fprintf('Completed32offset FFT probes on%dx%d preserved Single values\n',sz(1),sz(2));
