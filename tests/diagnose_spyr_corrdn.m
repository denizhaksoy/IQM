% Isolate highpass arithmetic without changing the official implementation.
args = argv(); root = args{1};
addpath(genpath('C:/HDR-VDP-3_Ref'));
[~, filt] = sp0Filters();
for label = {'ref', 'test'}
  name = label{1};
  fid = fopen(fullfile(root, [name '_jnd.txt']), 'r');
  dims = fscanf(fid, '%d', 2); v = fscanf(fid, '%f'); fclose(fid);
  img = single(reshape(v, dims(2), dims(1))');
  fid = fopen(fullfile(root, ['gpu_' name '_band0.txt']), 'r');
  fscanf(fid, '%d', 2); v = fscanf(fid, '%f'); fclose(fid);
  gpu = single(reshape(v, dims(2), dims(1))');
  ref = corrDn(img, filt, 'reflect1');
  fprintf('%s conv2(single,double) vs conv2(single,single): %g\n', name, max(abs(ref(:)-corrDn(img,single(filt))(:))));
  for axis = 0:1
    for reverse = 0:1
      for fused = 0:1
        acc = zeros(size(img), 'single');
        taps = 1:numel(filt); if reverse, taps = fliplr(taps); end
        for tap = taps
          if axis == 0
            fy = floor((tap-1)/9)+1; fx = mod(tap-1,9)+1;
          else
            fx = floor((tap-1)/9)+1; fy = mod(tap-1,9)+1;
          end
          yy = (0:dims(1)-1)+fy-5; xx = (0:dims(2)-1)+fx-5;
          yy = abs(yy); xx = abs(xx);
          yy(yy>=dims(1)) = 2*(dims(1)-1)-yy(yy>=dims(1));
          xx(xx>=dims(2)) = 2*(dims(2)-1)-xx(xx>=dims(2));
          sample = img(yy+1,xx+1); coeff = single(filt(fy,fx));
          if fused
            acc = single(double(acc)+double(sample)*double(coeff));
          else
            acc = acc + sample*coeff;
          end
        end
        dg = double(acc)-double(gpu); dr = double(acc)-double(ref);
        fprintf('%s axis=%d reverse=%d fused=%d GPUmax=%.17g GPUneq=%d OCTmax=%.17g OCTneq=%d\n', ...
          name, axis, reverse, fused, max(abs(dg(:))), nnz(dg), max(abs(dr(:))), nnz(dr));
      end
    end
  end
end
