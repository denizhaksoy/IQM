function [count,ppd]=hdrvdp_validation_band_count(root)
  count=5; path=fullfile(root,'band_count.txt');
  if exist(path,'file'),count=dlmread(path);endif
  assert(isscalar(count)&&any(count==[4,5]),'Unvalidated pyramid band count');
  ppd=30;path=fullfile(root,'pixels_per_degree.txt');
  if exist(path,'file'),ppd=dlmread(path);endif
  assert(isscalar(ppd)&&any(ppd==[30,60]),'Unvalidated pixels per degree');
endfunction
