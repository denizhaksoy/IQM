1;
% Native single-precision mask statistics from the actual exported inputs.
function image=read_single_map(path)
  f=fopen(path,'r'); assert(f>=0); dims=fscanf(f,'%d',2); values=fscanf(f,'%f'); fclose(f);
  assert(numel(values)==prod(dims)); image=single(reshape(values,dims(2),dims(1))');
endfunction
args=argv(); output=args{end}; assert(!exist(output,'file')); cases={};
for a=1:numel(args)-1
  f=fopen(args{a},'r'); assert(f>=0);
  while true
    line=fgetl(f); if !ischar(line), break; endif
    fields=strsplit(line,char(9)); assert(numel(fields)==2); name=fields{1}; root=fields{2};
    reference=read_single_map(fullfile(root,'ref_luminance.txt'));
    test=read_single_map(fullfile(root,'test_luminance.txt'));
    relative=abs(test-reference)./reference;
    addpath(fileparts(mfilename('fullpath')));[bands,ppd]=hdrvdp_validation_band_count(root);
    qf=fopen(fullfile(root,'quality.txt'),'r'); assert(qf>=0); quality=fscanf(qf,'%f'); fclose(qf); assert(numel(quality)==bands+3);
    selected=dlmread(fullfile(root,'base_native_selection.txt'));
    entry=struct('name',name,'export',root,'Q',quality(bands+2),'Q_JOD',quality(bands+3),...
      'input_height',rows(reference),'input_width',columns(reference),'band_count',bands,'pixels_per_degree',ppd,...
      'active_mask_pixels',nnz(relative>0.001),'equal_threshold_pixels',nnz(relative==0.001),...
      'relative_min',double(min(relative(:))),'relative_max',double(max(relative(:))),...
      'base_index_one_based',selected(1),'base_frequency',selected(2));
    cases{end+1}=entry;
    fprintf('%s ppd=%g Q=%.17g Q_JOD=%.17g active_mask=%d/%d index=%d frequency=%.17g\n',name,ppd,entry.Q,entry.Q_JOD,entry.active_mask_pixels,numel(reference),selected(1),selected(2));
  endwhile
  fclose(f);
endfor
f=fopen(output,'w'); assert(f>=0); fprintf(f,'%s\n',jsonencode(cases)); fclose(f);
