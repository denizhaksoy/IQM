1;
function z=read_complex(path)
  fid=fopen(path,'r'); assert(fid>=0); d=fscanf(fid,'%d',2); v=fscanf(fid,'%f'); fclose(fid);
  a=single(reshape(v,d(2),d(1))'); z=complex(a(:,1:2:end),a(:,2:2:end));
end
function compare(label,a,b)
  delta=double(a)-double(b); [worst,index]=max(abs(delta(:))); [y,x]=ind2sub(size(a),index);
  fprintf('%s max=%.17g RMSE=%.17g neq=%d worst=(%d,%d)\n',label,worst,sqrt(mean(abs(delta(:)).^2)),nnz(delta),y,x);
end
args=argv(); root=args{1};
column_first=false;
if exist(fullfile(root,'fft_axis_order.txt'),'file')
  column_first=strcmp(strtrim(fileread(fullfile(root,'fft_axis_order.txt'))),'column');
end
fprintf('Reported double FFTW planner=%s threads=%d; inspect native single separately\n',fftw('planner'),fftw('threads'));
for label={'ref','test'}
  name=label{1}; prefix=fullfile(root,[name '_mtf_']);
  input=read_complex([prefix 'stage0.txt']); gpu=read_complex([prefix 'stage1.txt']);
  filtered=read_complex([prefix 'stage2.txt']); inverse=read_complex([prefix 'stage3.txt']);
  if column_first
    input=input.'; gpu=gpu.'; filtered=filtered.';
    storage=fullfile(root,'fft_inverse_storage.txt');
    if ~exist(storage,'file') || strcmp(strtrim(fileread(storage)),'column'), inverse=inverse.'; end
  end
  official=fft2(input);
  official_real=fft2(real(input));
  rowfirst=fft(fft(input,[],2),[],1);
  colfirst=fft(fft(input,[],1),[],2);
  transposed=fft2(input.').';
  accurate=single(fft2(double(input)));
  compare([name ' GPU vs fft2'],gpu,official);
  compare([name ' GPU vs real-fft2'],gpu,official_real);
  compare([name ' real-fft2 vs complex-fft2'],official_real,official);
  fprintf('%s complex-input isreal=%d real-input isreal=%d\n',name,isreal(input),isreal(real(input)));
  compare([name ' GPU vs rowfirst'],gpu,rowfirst);
  compare([name ' GPU vs colfirst'],gpu,colfirst);
  compare([name ' GPU vs transposed'],gpu,transposed);
  compare([name ' fft2 vs rowfirst'],official,rowfirst);
  compare([name ' fft2 vs colfirst'],official,colfirst);
  compare([name ' GPU vs double-rounded'],gpu,accurate);
  compare([name ' fft2 vs double-rounded'],official,accurate);
  inverse_official=ifft2(filtered);
  inverse_rowfirst=ifft(ifft(filtered,[],2),[],1);
  inverse_colfirst=ifft(ifft(filtered,[],1),[],2);
  compare([name ' inverseGPU vs ifft2'],inverse,inverse_official);
  compare([name ' inverseGPU vs rowfirst'],inverse,inverse_rowfirst);
  compare([name ' inverseGPU vs colfirst'],inverse,inverse_colfirst);
  compare([name ' ifft2 vs rowfirst'],inverse_official,inverse_rowfirst);
  compare([name ' ifft2 vs colfirst'],inverse_official,inverse_colfirst);
end
fprintf('Single FFTW wisdom after these transforms:\n%s\n',fftw('swisdom'));
