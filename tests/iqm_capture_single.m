function iqm_capture_single(tag,value)
  global IQM_SINGLE_CAPTURE_ROOT;
  assert(~isempty(IQM_SINGLE_CAPTURE_ROOT));
  indices=unique(round(linspace(1,numel(value),min(16,numel(value)))));
  entry=struct('tag',tag,'class',class(value),'shape',size(value),...
    'layout','Octave-column-major','samples',double(value(indices)),...
    'sample_hex',num2hex(value(indices)));
  if strcmp(tag,'matrix')||~isempty(strfind(tag,'_native'))||...
     ~isempty(strfind(tag,'_optical'))||~isempty(strfind(tag,'_lmsr'))||...
     ~isempty(strfind(tag,'_adaptation'))||~isempty(strfind(tag,'_response'))||...
     ~isempty(strfind(tag,'_band'))
    if isa(value,'single'),kind='single';ext='.f32';else,kind='double';ext='.f64';end
    f=fopen(fullfile(IQM_SINGLE_CAPTURE_ROOT,[tag ext]),'wb','ieee-le');assert(f>=0);
    fwrite(f,value,kind);fclose(f);
  end
  f=fopen(fullfile(IQM_SINGLE_CAPTURE_ROOT,'classes.jsonl'),'at');
  assert(f>=0);fprintf(f,'%s\n',jsonencode(entry));fclose(f);
end
