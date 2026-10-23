% Establish native Octave rounding and operation ordering on exported inputs.
run(fullfile(fileparts(mfilename('fullpath')),'diagnose_jnd_path.m'));
for k = 1:2
  name = labels{k};
  lum = read_image(fullfile(root,[name '_luminance.txt']));
  if exist(fullfile(root,[name '_retinal.txt']),'file')
    lum = read_image(fullfile(root,[name '_retinal.txt']));
  end
  adapt = read_image(fullfile(root,[name '_adaptation.txt']));
  logs = log(adapt(:));
  seq = single(0); for i=1:numel(logs), seq = seq+logs(i); end
  rowlogs = log(adapt'(:));
  rowseq = single(0); for i=1:numel(rowlogs), rowseq = rowseq+rowlogs(i); end
  geo = geomean(adapt(:));
  fprintf('%s logs sum=%.17g sequentialCol=%.17g sequentialRow=%.17g geomean=%.17g\n',name,sum(logs),seq,rowseq,geo);
  dref = pupil_d_unified(geo,area,28); dage = pupil_d_unified(geo,area,age);
  reduction = dage.^2 ./ dref.^2;
  before = lum(:)*matrix([1 2 4]);
  after = max(min(before,1e10),1e-8)*reduction;
  query = log10(max(min(after,1e5),1e-5));
  channels = zeros(size(query),'single');
  for ch=1:3
    table = cones; if ch==3, table=rods; end
    channels(:,ch) = interp1(log_l,table,query(:,ch));
  end
  response = (channels(:,1)+channels(:,2))+channels(:,3);
  % Full native preprocessing from the official L+M adaptation input, MTF off.
  official_adapt = hdrvdp_local_adapt(reshape(before(:,1)+before(:,2),size(lum)),ppd);
  official_geo = geomean(official_adapt(:));
  official_dr = pupil_d_unified(official_geo,area,28);
  official_da = pupil_d_unified(official_geo,area,age);
  official_reduction = official_da.^2 ./ official_dr.^2;
  official_query = log10(max(min(max(min(before,1e10),1e-8)*official_reduction,1e5),1e-5));
  official_channels = zeros(size(official_query),'single');
  for ch=1:3
    table=cones; if ch==3, table=rods; end
    official_channels(:,ch)=interp1(log_l,table,official_query(:,ch));
  end
  official_response=(official_channels(:,1)+official_channels(:,2))+official_channels(:,3);
  integrated=read_image(fullfile(root,[name '_jnd.txt']));
  official_delta=double(integrated(:))-double(official_response);
  fprintf('%s official-adaptation native JND max=%.17g mismatches=%d pupilDifference=%.17g\n', ...
    name,max(abs(official_delta)),nnz(official_delta),params(k)-double(official_reduction));
  if native_path
    gpu = read_image(fullfile(root,[name '_jnd.txt']));
    delta = double(gpu(:))-double(response);
    fprintf('%s integrated native JND max=%.17g mismatches=%d pupilDifference=%.17g\n',name,max(abs(delta)),nnz(delta),params(k)-double(reduction));
    assert(nnz(delta)==0 && params(k)==double(reduction),'Integrated native single mismatch');
  end
  fprintf('%s native classes: geo=%s pupil=%s LMSR=%s log=%s interp=%s response=%s reduction=%.17g\n', ...
    name,class(geo),class(reduction),class(before),class(query),class(interp1(log_l,cones,query(:,1))),class(response),reduction);
  % Compare interp1 with a straightforward float linear interpolation.
  for ch=1:3
    table = cones(:); if ch==3, table=rods(:); end
    axis_values = log_l(:);
    q = query(:,ch); indices = lookup(axis_values,q); indices = min(max(indices,1),2047);
    dx = single(axis_values(indices+1)-axis_values(indices));
    slope = single(diff(table)); slope = slope(indices)./dx;
    candidate = slope.*(q-single(axis_values(indices)))+single(table(indices));
    fprintf('%s ch%d float-slope candidate max=%.17g neq=%d\n',name,ch,max(abs(double(candidate)-double(channels(:,ch)))),nnz(candidate!=channels(:,ch)));
  end
  trace = [double(before),double(after),double(query),double(channels),double(response)];
  dlmwrite(fullfile(root,[name '_native_jnd_trace.csv']),trace,'precision','%.17g');
  dlmwrite(fullfile(root,[name '_native_pupil_trace.csv']),[double(geo),double(dref),double(dage),double(reduction)],'precision','%.17g');
end
