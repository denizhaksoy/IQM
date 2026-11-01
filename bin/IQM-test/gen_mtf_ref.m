% Generate official HDR-VDP-3 Optical MTF reference for 64x64 deterministic input
% Using exact same code path as official reference

addpath('C:/HDR-VDP-3_Ref/utils');
addpath('C:/HDR-VDP-3_Ref/matlabPyrTools');

% Manual padarray implementation for 'replicate', 'post'
function Y = my_padarray(X, pad_size, pad_value, direction)
    if strcmp(direction, 'post')
        [H, W] = size(X);
        pad_h = pad_size(1);
        pad_w = pad_size(2);
        Y = zeros(H + pad_h, W + pad_w);
        Y(1:H, 1:W) = X;
        if pad_w > 0
            Y(1:H, W+1:end) = repmat(X(:, end), 1, pad_w);
        end
        if pad_h > 0
            Y(H+1:end, :) = repmat(Y(H, :), pad_h, 1);
        end
    else
        error('Only post padding supported');
    end
end

% fast_conv_fft - exact copy from official reference
function Y = my_fast_conv_fft(X, fH, pad_value)
    pad_size = (size(fH)-size(X));
    fX = fft2(my_padarray(X, pad_size, pad_value, 'post'));
    Yl = real(ifft2(fX.*fH, size(fX,1), size(fX,2)));
    Y = Yl(1:size(X,1),1:size(X,2));
end

% Exact deterministic input matching GPU test
% GPU: data[y * width + x] = (x * height + y + 1) / N * 100.0f
width = 64;
height = 64;
N = width * height;
img = zeros(height, width);
for y = 1:height
    for x = 1:width
        img(y, x) = ((x-1) * height + y) / N * 100.0;
    end
end

% Official HDR-VDP-3 parameters (default from hdrvdp_parse_options.m)
metric_par.mtf = 'hdrvdp';
par = [0.061466549455263 0.99727370023777070];
metric_par.mtf_params_a = [par(2)*0.426 par(2)*0.574 (1-par(2))*par(1) (1-par(2))*(1-par(1))];
metric_par.mtf_params_b = [0.028 0.37 37 360];
metric_par.pix_per_deg = 30;  % PPD = 30
metric_par.pad_value = 'symmetric';  % from hdrvdp_determine_padding with surround='none'

% Create rho2 for 2x image size (128x128)
rho2 = create_cycdeg_image([height width]*2, metric_par.pix_per_deg, false);

% Generate MTF filter at 2x size
mtf_filter = hdrvdp_mtf(rho2, metric_par);

% Apply Optical MTF via fast_conv_fft with replicate padding
L_O = my_fast_conv_fft(img, mtf_filter, 'replicate');

% Clamp to [1e-5, 1e10]
L_O = max(min(L_O, 1e10), 1e-5);

% Save reference output (row-major for GPU comparison)
fid = fopen('C:/temp/oct_mtf_output_rowmajor.txt', 'w');
for y = 1:height
    for x = 1:width
        fprintf(fid, '%.10f\n', L_O(y, x));
    end
end
fclose(fid);

% Also save MTF filter for verification
fid = fopen('C:/temp/oct_mtf_filter_rowmajor.txt', 'w');
for y = 1:128
    for x = 1:128
        fprintf(fid, '%.10f\n', mtf_filter(y, x));
    end
end
fclose(fid);

% Save padded input for verification
pad_size = (size(mtf_filter)-size(img));
img_padded = my_padarray(img, pad_size, 'replicate', 'post');
fid = fopen('C:/temp/oct_mtf_input_padded_rowmajor.txt', 'w');
for y = 1:128
    for x = 1:128
        fprintf(fid, '%.10f\n', img_padded(y, x));
    end
end
fclose(fid);

fprintf('Reference generated successfully.\n');
fprintf('Output size: %dx%d\n', width, height);
fprintf('MTF filter size: %dx%d\n', size(mtf_filter,2), size(mtf_filter,1));
fprintf('Padded input size: %dx%d\n', size(img_padded,2), size(img_padded,1));
fprintf('Output TL: %.10f\n', L_O(1,1));
fprintf('Output TR: %.10f\n', L_O(1,width));
fprintf('Output BL: %.10f\n', L_O(height,1));
fprintf('Output BR: %.10f\n', L_O(height,width));