function count=hdrvdp_validation_band_count(root)
  count=5; path=fullfile(root,'band_count.txt');
  if exist(path,'file'),count=dlmread(path);endif
  assert(isscalar(count)&&any(count==[4,5]),'Unvalidated pyramid band count');
endfunction
