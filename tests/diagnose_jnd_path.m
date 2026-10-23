% Stage diagnostics; no reference implementation files are modified.
1;
function img = read_image(filename)
  fid = fopen(filename,'r'); assert(fid >= 0);
  dims = fscanf(fid,'%d',2); values = fscanf(fid,'%f'); fclose(fid);
  assert(numel(values)==prod(dims));
  img = single(reshape(values,dims(2),dims(1))');
end
function p = jnd_response(luminance, reduction, matrix, log_l, cones, rods)
  lmr = double(luminance(:)) * matrix([1 2 4]) * reduction;
  lmr = min(max(lmr,1e-5),1e5);
  p = interp1(log_l,cones,log10(lmr(:,1))) ...
    + interp1(log_l,cones,log10(lmr(:,2))) ...
    + interp1(log_l,rods,log10(lmr(:,3)));
  p = reshape(single(p),size(luminance));
end
args = argv(); root = args{1}; pkg load image; pkg load statistics;
addpath(fileparts(mfilename('fullpath')));[~,ppd]=hdrvdp_validation_band_count(root);
addpath(genpath('C:/HDR-VDP-3_Ref'));
params = dlmread(fullfile(root,'pupil.txt')); area = params(3); age = params(4);
native_path = exist(fullfile(root,'pupil_precision.txt'),'file') != 0;
% Reconstruct the D65 single-channel spectral transform using official data.
[lambda, spectral] = load_spectral_resp('C:/HDR-VDP-3_Ref/data/log_cone_smith_pokorny_1975.csv');
spectral(spectral==0) = min(spectral(:)); spectral = 10.^spectral;
[~, spectral(:,4)] = load_spectral_resp('C:/HDR-VDP-3_Ref/data/cie_scotopic_lum.txt');
[~, emission] = load_spectral_resp('C:/HDR-VDP-3_Ref/data/d65.csv');
if columns(emission)>1, emission = sum(emission,2); end
% Default do_aod age-dependent optical density, from hdrvdp_visual_pathway.m.
lam = 400:10:650;
TL1 = [0.600 0.510 0.433 0.377 0.327 0.295 0.267 0.233 0.207 0.187 0.167 0.147 0.133 0.120 0.107 0.093 0.080 0.067 0.053 0.040 0.033 0.027 0.020 0.013 0.007 0.000];
TL2 = [1.000 0.583 0.300 0.116 0.033 0.005 zeros(1,20)];
if age <= 60, OD = TL1*(1+0.02*(age-32))+TL2;
else, OD = TL1*(1.56+0.0667*(age-60))+TL2; end
trans_filter = 10.^-interp1(lam,OD-(TL1+TL2),min(max(lambda,lam(1)),lam(end)),'pchip');
emission = emission.*trans_filter';
matrix = trapz(lambda,spectral.*emission*683.002); matrix = matrix/sum(matrix(1:2));
fprintf('D65 LMSR: %.17g %.17g %.17g %.17g\n',matrix);
metric_par.csf_sa = [315.98 6.7977 1.6008 0.25534];
metric_par.csf_sr = [1.1732 1.32 1.095 0.5547 2.9899 1.8];
c_l = logspace(-5,5,2048); log_l = log10(c_l);
s_A = hdrvdp_joint_rod_cone_sens(c_l,metric_par);
s_R = hdrvdp_rod_sens(c_l,metric_par);
s_C = 0.5*interp1(c_l,max(s_A-s_R,1e-3),min(c_l*2,c_l(end)));
cones = cumtrapz(log_l,s_C*log(10))*10^0.203943775672;
rods = cumtrapz(log_l,s_R*log(10))*10^0.203943775672;
for k = 1:2
  labels = {'ref','test'}; name = labels{k};
  lum = read_image(fullfile(root,[name '_luminance.txt']));
  if exist(fullfile(root,[name '_retinal.txt']),'file')
    lum = read_image(fullfile(root,[name '_retinal.txt']));
  end
  adapt = read_image(fullfile(root,[name '_adaptation.txt']));
  gpu_jnd = read_image(fullfile(root,[name '_jnd.txt']));
  oct_adapt = hdrvdp_local_adapt(lum,ppd);
  delta = double(adapt)-double(oct_adapt);
  geo_gpu = exp(mean(log(double(adapt(:)))));
  geo_oct = exp(mean(log(double(oct_adapt(:)))));
  expected_gpu = (pupil_d_unified(geo_gpu,area,age)/pupil_d_unified(geo_gpu,area,28))^2;
  expected_oct = (pupil_d_unified(geo_oct,area,age)/pupil_d_unified(geo_oct,area,28))^2;
  native_geo = geomean(oct_adapt(:));
  native_da = pupil_d_unified(native_geo,area,age);
  native_dr = pupil_d_unified(native_geo,area,28);
  native_pupil = native_da.^2 ./ native_dr.^2;
  fprintf('%s adaptation max=%.17g RMSE=%.17g geoGPU=%.17g geoOct=%.17g\n',name,max(abs(delta(:))),sqrt(mean(delta(:).^2)),geo_gpu,geo_oct);
  fprintf('%s pupil actual=%.17g expectedOnGPUAdapt=%.17g expectedOnOctAdapt=%.17g\n',name,params(k),expected_gpu,expected_oct);
  if ~native_path
    assert(abs(params(k)-expected_gpu)<1e-14,'Pupil does not use the adaptation geometric mean');
  end
  fprintf('%s native geomean class=%s value=%.17g pupil=%.17g\n',name,class(native_geo),native_geo,native_pupil);
  for mode = 1:3
    reductions = [params(k),expected_oct,double(native_pupil)];
    expected = jnd_response(lum,reductions(mode),matrix,log_l,cones,rods);
    delta = double(gpu_jnd)-double(expected);
    fprintf('%s JND mode=%d max=%.17g RMSE=%.17g mismatches=%d\n',name,mode,max(abs(delta(:))),sqrt(mean(delta(:).^2)),nnz(delta));
    if mode == 1 && ~native_path, assert(nnz(delta)==0,'JND mismatch at identical pupil parameters'); end
  end
end
