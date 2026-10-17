1;
function z=read_complex(path)
  fid=fopen(path,'r'); assert(fid>=0); d=fscanf(fid,'%d',2); v=fscanf(fid,'%f'); fclose(fid);
  assert(numel(v)==prod(d)); a=single(reshape(v,d(2),d(1))'); z=complex(a(:,1:2:end),a(:,2:2:end));
end
args=argv(); probe=args{1}; source=args{2}; addpath(probe);
output=probe;
if numel(args)>2
  output=args{3}; assert(~exist(output,'dir'),'Probe output directory must be new');
  assert(mkdir(output));
end
column_first=strcmp(strtrim(fileread(fullfile(source,'fft_axis_order.txt'))),'column');
fprintf('Octave %s reported double FFTW planner=%s threads=%d\n',version(),fftw('planner'),fftw('threads'));
for label={'ref','test'}
  for kind={'mtf','la'}
    name=label{1}; mode=kind{1};
    prepared=read_complex(fullfile(source,[name '_' mode '_stage0.txt']));
    filtered=read_complex(fullfile(source,[name '_' mode '_stage2.txt']));
    if column_first, prepared=prepared.'; filtered=filtered.'; end
    for inverse=0:1
      if inverse, input=filtered; expected=ifft2(input); direction='inverse';
      else, input=real(prepared); expected=fft2(input); direction='forward'; end
      [actual,plan,counts,single_threads]=inspect_fftw_native(input,logical(inverse));
      delta=double(actual)-double(expected);
      fprintf('%s %s %s max=%.17g neq=%d adds=%g muls=%g fmas=%g\n', ...
        name,mode,direction,max(abs(delta(:))),nnz(delta),counts);
      fprintf('%s\n',plan);
      fprintf('Native single FFTW threads=%d\n',single_threads);
      fid=fopen(fullfile(output,'single_threads.txt'),'w'); fprintf(fid,'%d\n',single_threads); fclose(fid);
      fid=fopen(fullfile(output,[name '_' mode '_' direction '_plan.txt']),'w');
      fprintf(fid,'%s\n',plan); fclose(fid);
      assert(all(isfinite(delta(:))) && nnz(delta)==0,'Native wrapper probe differs from fft2/ifft2');
    end
  end
end
