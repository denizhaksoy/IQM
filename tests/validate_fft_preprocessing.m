% Fresh official MTF + local adaptation + native pupil/JND reference.
run(fullfile(fileparts(mfilename('fullpath')),'trace_jnd_single.m'));
metric_par.mtf='hdrvdp';
par=[0.061466549455263 0.99727370023777070];
metric_par.mtf_params_a=[par(2)*0.426 par(2)*0.574 (1-par(2))*par(1) (1-par(2))*(1-par(1))];
metric_par.mtf_params_b=[0.028 0.37 37 360];
metric_par.surround='none'; metric_par.use_gpu=false;
addpath(fileparts(mfilename('fullpath')));[band_count,ppd]=hdrvdp_validation_band_count(root);
full_mismatches=0;
% The preceding JND diagnostics also execute FFTs. Opt-in reset reproduces
% independent rectangular fixtures instead of reusing their cached FFTW plans.
if any(strcmp(args,'--fresh-fft-plans'))
  fftw('threads',3);fftw('swisdom','');
  fprintf('Full preprocessing FFTW threads=%d planner=%s;single wisdom reset\n',fftw('threads'),fftw('planner'));
end
for k=1:2
  name=labels{k}; original=read_image(fullfile(root,[name '_luminance.txt']));
  metric_par.pad_value=hdrvdp_determine_padding(original,metric_par);
  rho=create_cycdeg_image(size(original)*2,ppd,false);
  filter=hdrvdp_mtf(rho,metric_par);
  retinal=clamp(fast_conv_fft(original,filter,metric_par.pad_value),1e-5,1e10);
  before=retinal(:)*matrix([1 2 4]); before=clamp(before,1e-8,1e10);
  adapt_input=reshape(before(:,1)+before(:,2),size(original));
  adaptation=hdrvdp_local_adapt(adapt_input,ppd);
  geo=geomean(adaptation(:));
  dr=pupil_d_unified(geo,area,28); da=pupil_d_unified(geo,area,age);
  reduction=da.^2 ./ dr.^2;
  query=log10(clamp(before*reduction,1e-5,1e5));
  channels=zeros(size(query),'single');
  for ch=1:3
    table=cones; if ch==3, table=rods; end
    channels(:,ch)=interp1(log_l,table,query(:,ch));
  end
  response=reshape((channels(:,1)+channels(:,2))+channels(:,3),size(original));
  stage_names={'retinal','adapt_input','adaptation','jnd'};
  official={retinal,adapt_input,adaptation,response};
  fprintf('%s official MTF padding=%s\n',name,metric_par.pad_value);
  for stage=1:4
    gpu=read_image(fullfile(root,[name '_' stage_names{stage} '.txt']));
    delta=double(gpu)-double(official{stage}); assert(all(isfinite(delta(:))));
    full_mismatches=full_mismatches+nnz(delta);
    fprintf('%s full %s max=%.17g RMSE=%.17g mismatches=%d\n',name,stage_names{stage},max(abs(delta(:))),sqrt(mean(delta(:).^2)),nnz(delta));
    fid=fopen(fullfile(root,[name '_oct_full_' stage_names{stage} '.txt']),'w');
    fprintf(fid,'%d %d\n',size(original)); fprintf(fid,'%.17g\n',official{stage}(:)); fclose(fid);
  end
  fprintf('%s full pupil C++=%.17g Octave=%.17g diff=%.17g\n',name,params(k),reduction,params(k)-double(reduction));
  [pyr,pind]=buildSpyr(response,band_count-2,'sp0Filters','reflect1');
  for b=0:band_count-1
    band=pyrBand(pyr,pind,b+1);
    gpu=read_image(fullfile(root,sprintf('gpu_%s_band%d.txt',name,b)));
    for centered=0:double(b==band_count-1)
      g=gpu; o=band;
      if centered, g=g-single(mean(double(g(:)))); o=o-mean(o(:)); end
      delta=double(g)-double(o);
      fprintf('%s full band%d centered=%d max=%.17g RMSE=%.17g mismatches=%d\n',name,b,centered,max(abs(delta(:))),sqrt(mean(delta(:).^2)),nnz(delta));
    end
  end
end
fprintf('Full preprocessing stage mismatches=%d (zero required for exact parity)\n',full_mismatches);
if numel(args)>1 && strcmp(args{2},'--strict')
  assert(full_mismatches==0,'Full preprocessing does not yet match exactly');
end
