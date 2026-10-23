args = argv(); root = args{1};
addpath(genpath('C:/HDR-VDP-3_Ref'));
[lo0, hi0, lo, bf] = sp0Filters(); bf = reshape(bf,9,9);
for label = {'ref', 'test'}
  name = label{1};
  fid = fopen(fullfile(root, [name '_jnd.txt']), 'r');
  dims = fscanf(fid, '%d', 2); v = fscanf(fid, '%f'); fclose(fid);
  img = single(reshape(v,dims(2),dims(1))');
  current = corrDn(img,lo0);
  inputs = {img,current}; filters = {hi0,lo0}; names = {'hi0','lo0'};
  for level = 0:2
    inputs{end+1} = current; filters{end+1} = bf; names{end+1} = sprintf('orient%d',level);
    inputs{end+1} = current; filters{end+1} = lo; names{end+1} = sprintf('down%d',level);
    current = corrDn(current,lo,'reflect1',[2 2]);
  end
  for stage = 1:numel(inputs)
    img = inputs{stage}; filt = filters{stage}; n = size(filt,1); radius = floor(n/2);
    target = corrDn(img,filt); dims = size(img);
    for axis = 0:1
      for reverse = 0:1
        for fused = 0:1
          acc = zeros(size(img),'single'); taps = 1:numel(filt);
          if reverse, taps = fliplr(taps); end
          for tap = taps
            if axis == 0
              fy = floor((tap-1)/n)+1; fx = mod(tap-1,n)+1;
            else
              fx = floor((tap-1)/n)+1; fy = mod(tap-1,n)+1;
            end
            yy = abs((0:dims(1)-1)+fy-1-radius); xx = abs((0:dims(2)-1)+fx-1-radius);
            yy(yy>=dims(1)) = 2*(dims(1)-1)-yy(yy>=dims(1));
            xx(xx>=dims(2)) = 2*(dims(2)-1)-xx(xx>=dims(2));
            sample = img(yy+1,xx+1); coeff = single(filt(fy,fx));
            if fused, acc = single(double(acc)+double(sample)*double(coeff));
            else, acc = acc + sample*coeff; end
          end
          delta = double(acc)-double(target);
          fprintf('%s %s %dx%d axis=%d reverse=%d fused=%d max=%.17g neq=%d\n', ...
            name,names{stage},dims,axis,reverse,fused,max(abs(delta(:))),nnz(delta));
        end
      end
    end
  end
end
