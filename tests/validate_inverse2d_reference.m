1;
function values=read_matrices(path)
  fid=fopen(path,'r'); assert(fid>=0);
  dims=fscanf(fid,'%d',2); raw=single(fscanf(fid,'%f')); fclose(fid);
  assert(dims(2)==256 && mod(dims(1),128)==0 && numel(raw)==prod(dims));
  values=reshape(complex(raw(1:2:end),raw(2:2:end)),128,128,[]);
end
args=argv(); source=args{1}; addpath(args{2});
input=read_matrices(fullfile(source,'input128.txt'));
expected=read_matrices(fullfile(source,'inverse128.txt'));
assert(isequal(size(input),size(expected)));
for m=1:size(input,3)
  actual=ifft2(input(:,:,m));
  [native,plan,counts,threads]=inspect_fftw_native(input(:,:,m),true);
  assert(threads==22,'Reference fixtures target native single threads=22');
  delta=double(actual)-double(expected(:,:,m));
  fprintf('matrix=%d complex=%d max=%.17g mismatches=%d native_threads=%d\n', ...
    m,numel(actual),max(abs(delta(:))),nnz(delta),threads);
  assert(all(isfinite(delta(:))) && nnz(delta)==0,'DLL oracle differs from native Octave ifft2');
  assert(isequal(native,actual),'Native probe differs from Octave ifft2');
end
fprintf('Normalized inverse2d CPU oracle vs native Octave: PASS\n');
