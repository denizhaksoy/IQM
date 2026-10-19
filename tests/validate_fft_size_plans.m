% Compare independent native-DLL plans with actual Octave FFT outputs.
1;
function image=read_complex_map(path)
  f=fopen(path,'r'); assert(f>=0,path); dims=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  assert(numel(v)==prod(dims)); image=single(v);
endfunction
args=argv(); reference=args{1}; candidates=args{2}; records={}; exact=0;
for label={'ref','test'}
  for threads=[1,3]
    for alignment={'aligned','unaligned'}
      for direction={'forward','inverse'}
        tag=sprintf('%s_threads%d_%s_%s',label{1},threads,alignment{1},direction{1});
        native=read_complex_map(fullfile(reference,[label{1} '_native_' direction{1} '.txt']));
        candidate=read_complex_map(fullfile(candidates,[tag '.txt']));
        assert(numel(native)==numel(candidate)); delta=double(candidate)-double(native);
        count=nnz(delta); if count==0,exact+=1;endif
        fprintf('%s values=%d mismatches=%d max=%.17g\n',tag,numel(delta),count,max(abs(delta)));
        records{end+1}=struct('tag',tag,'values',numel(delta),'mismatches',count,'max',max(abs(delta)));
      endfor
    endfor
  endfor
endfor
f=fopen(fullfile(candidates,'comparison.json'),'w'); fputs(f,jsonencode(struct('scope','Native DLL plan comparison;not GPU','exact_cases',exact,'cases',{records}))); fclose(f);
fprintf('Plan comparisons=%d exact=%d\n',numel(records),exact);
% Require the measured native scalar path for both input labels/directions.
for k=1:numel(records)
  r=records{k}; if !isempty(strfind(r.tag,'_unaligned_')),assert(r.mismatches==0,'Scalar plan does not match native FFT');endif
endfor
