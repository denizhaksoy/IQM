% Compare exact float32 JND exports against unmodified official pyramid code.
args = argv();
root = args{1};
addpath(genpath('C:/HDR-VDP-3_Ref'));
fprintf('Version: %g\ncorrDn: %s\n', hdrvdp_version(), which('corrDn'));
total_mismatches = 0;
for label = {'ref', 'test'}
  name = label{1};
  fid = fopen(fullfile(root, [name '_jnd.txt']), 'r');
  dims = fscanf(fid, '%d', 2); values = fscanf(fid, '%f'); fclose(fid);
  assert(numel(values) == prod(dims));
  img = single(reshape(values, dims(2), dims(1))');
  fprintf('%s transpose-difference=%.17g\n', name, max(abs(double(img(:))-double(img'(:)))));
  [pyr, pind] = buildSpyr(img, 3, 'sp0Filters', 'reflect1');
  fprintf('%s input=%s pyramid=%s\n', name, class(img), class(pyr));
  for b = 0:4
    band = pyrBand(pyr, pind, b+1);
    fid = fopen(fullfile(root, sprintf('oct_single_%s_band%d.txt', name, b)), 'w');
    fprintf(fid, '%d %d\n', size(band)); fprintf(fid, '%.17g\n', band(:)); fclose(fid);
    fid = fopen(fullfile(root, sprintf('gpu_%s_band%d.txt', name, b)), 'r');
    gd = fscanf(fid, '%d', 2); gv = fscanf(fid, '%f'); fclose(fid);
    assert(isequal(gd(:)', size(band)) && numel(gv) == numel(band));
    gpu = single(reshape(gv, gd(2), gd(1))');
    assert(all(isfinite(gpu(:))) && all(isfinite(band(:))));
    for centered = 0:double(b == 4)
      g = gpu; o = band;
      if centered
        % Match current C++ double accumulation, float mean, float subtraction.
        g = g - single(mean(double(g(:))));
        o = o - mean(o(:));
      end
      delta = double(g(:)) - double(o(:));
      total_mismatches = total_mismatches + nnz(delta);
      idx = find(delta != 0, 1); if isempty(idx), idx = 0; end
      fprintf('%s band%d centered=%d n=%d MaxAbsErr=%.17g RMSE=%.17g mismatches=%d first=%d meanGPU=%.17g meanOct=%.17g\n', ...
        name, b, centered, numel(delta), max(abs(delta)), sqrt(mean(delta.^2)), nnz(delta), idx, mean(double(g(:))), mean(double(o(:))));
    end
  end
end
assert(total_mismatches == 0, sprintf('Parity failed: %d mismatches', total_mismatches));
