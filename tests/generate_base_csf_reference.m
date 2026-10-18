1;
% Native base-band frequencies at64x64/30ppd, age24, single adaptation.
args=argv(); output=args{1}; assert(!exist(output,'dir')); assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref'; if numel(args)>1, reference=args{2}; endif
addpath(genpath(reference)); assert(hdrvdp_version()==3.07,'Official HDR-VDP-3 v3.0.7 required');
p=hdrvdp_parse_options('quality',{}); p.use_gpu=false; p.age=24;
rho=unique(create_cycdeg_image([8,8],3.75));
luminance=single(logspace(-5,5,1024));
for node=single(p.csf_lums(:)')
  bits=typecast(node,'uint32');
  luminance=[luminance,typecast(bits-uint32(1),'single'),node,typecast(bits+uint32(1),'single')];
endfor
luminance=unique(luminance);
grid=log10(p.csf_lums);
q=clamp(log10(luminance),grid(1),grid(end));
trace=zeros(numel(luminance),5,'single'); trace(:,1)=q(:);
for k=1:4, trace(:,k+1)=interp1(grid,p.csf_params(:,k+1),q(:)); endfor
tf=fopen(fullfile(output,'base_csf_parameters.txt'),'w'); assert(tf>=0);
fprintf(tf,'%d\n',numel(luminance)); fprintf(tf,'%.9g\n',luminance); fprintf(tf,'%.9g\n',trace'); fclose(tf);
f=fopen(fullfile(output,'csf_single.txt'),'w'); assert(f>=0);
nf=fopen(fullfile(output,'base_noise.txt'),'w'); assert(nf>=0);
for frequency=rho(:)'
  % Matching shapes avoid the reference's scalar-rho/vector-lum mask behavior.
  values=hdrvdp_ncsf(repmat(frequency,numel(luminance),1),luminance(:),p);
  assert(isa(values,'double'));
  noise=1./values; neural=single(noise.^(2*(10^p.mask_p)));
  for k=1:numel(luminance)
    fprintf(f,'%.17g %.17g 24 %d %.17g\n',frequency,double(luminance(k)),p.do_aesl,values(k));
    fprintf(nf,'%.17g %.17g %.17g %.9g\n',frequency,double(luminance(k)),noise(k),neural(k));
  endfor
endfor
fclose(f);
fclose(nf);
fprintf('Native base CSF grid:%d frequencies x%d luminances =%d cases\n',numel(rho),numel(luminance),numel(rho)*numel(luminance));
