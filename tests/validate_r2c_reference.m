1;
function values=read_vectors(path)
  fid=fopen(path,'r'); assert(fid>=0);
  dims=fscanf(fid,'%d',2); raw=single(fscanf(fid,'%f')); fclose(fid);
  assert(dims(2)==256 && numel(raw)==prod(dims));
  values=reshape(complex(raw(1:2:end),raw(2:2:end)),128,[]);
end
args=argv(); source=args{1};
input=read_vectors(fullfile(source,'input128.txt'));
expected=read_vectors(fullfile(source,'forward128.txt'));
assert(isequal(size(input),size(expected)) && nnz(imag(input))==0);
actual=fft(real(input),[],1);
assert(isa(actual,'single'));
delta=double(actual)-double(expected);
fprintf('R2C oracle vs Octave fft: transforms=%d complex=%d max=%.17g mismatches=%d\n', ...
  size(input,2),numel(actual),max(abs(delta(:))),nnz(delta));
assert(all(isfinite(delta(:))) && nnz(delta)==0,'R2C DLL oracle differs from native Octave fft');
fprintf('R2C reference: PASS\n');
