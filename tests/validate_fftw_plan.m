1;
function z=read_complex(path)
  fid=fopen(path,'r'); assert(fid>=0); d=fscanf(fid,'%d',2); values=fscanf(fid,'%f'); fclose(fid);
  assert(numel(values)==prod(d)); a=single(reshape(values,d(2),d(1))');
  z=complex(a(:,1:2:end),a(:,2:2:end));
end
args=argv(); plan_root=args{1}; source=strtrim(fileread(fullfile(plan_root,'source.txt')));
column_first=strcmp(strtrim(fileread(fullfile(source,'fft_axis_order.txt'))),'column');
fprintf('Octave reported double planner=%s threads=%d (not native single thread count)\n',fftw('planner'),fftw('threads'));
thread_counts=[3 1];
native_threads=0; native_pass=true;
if exist(fullfile(plan_root,'single_threads.txt'),'file')
  native_threads=dlmread(fullfile(plan_root,'single_threads.txt'));
  thread_counts=unique([native_threads 3 1]);
end
forward_match=false; inverse_match=false;
for label={'ref','test'}
  name=label{1};
  prepared=read_complex(fullfile(source,[name '_mtf_stage0.txt']));
  filtered=read_complex(fullfile(source,[name '_mtf_stage2.txt']));
  if column_first, prepared=prepared.'; filtered=filtered.'; end
  expected_forward=fft2(real(prepared)); expected_inverse=ifft2(filtered);
  for threads=thread_counts
    for alignment={'aligned','unaligned'}
      tag=sprintf('%s_threads%d_%s',name,threads,alignment{1});
      for direction={'forward','inverse'}
        kind=direction{1}; actual=read_complex(fullfile(plan_root,[tag '_' kind '.txt']));
        expected=expected_forward; if strcmp(kind,'inverse'), expected=expected_inverse; end
        delta=double(actual)-double(expected); assert(all(isfinite(delta(:))));
        if threads==native_threads && strcmp(alignment{1},'unaligned')
          native_pass=native_pass && nnz(delta)==0;
        end
        fprintf('%s %s max=%.17g RMSE=%.17g neq=%d\n',tag,kind,max(abs(delta(:))),sqrt(mean(abs(delta(:)).^2)),nnz(delta));
        if strcmp(kind,'forward') && threads==1 && strcmp(alignment{1},'unaligned')
          rows_with_error=find(any(delta!=0,2));
          fprintf('%s forward mismatch rows:',tag); fprintf(' %d',rows_with_error); fprintf('\n');
        end
        if strcmp(kind,'forward'), forward_match=forward_match || nnz(delta)==0;
        else, inverse_match=inverse_match || nnz(delta)==0; end
      end
    end
  end
end
fprintf('Exact native forward candidate=%d inverse candidate=%d\n',forward_match,inverse_match);
fid=fopen(fullfile(plan_root,'native_wisdom.txt'),'w');
fprintf(fid,'%s',fftw('swisdom')); fclose(fid);
if numel(args)>1 && strcmp(args{2},'--strict-native')
  assert(native_threads>0 && native_pass,'Configured native unaligned plan differs from Octave');
end
