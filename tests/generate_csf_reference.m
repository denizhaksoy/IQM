1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref'; if numel(args)>1, reference=args{2}; endif
addpath(genpath(reference)); assert(hdrvdp_version()==3.07,'Official HDR-VDP-3 v3.0.7 required');
base=hdrvdp_parse_options('quality',{}); base.use_gpu=false;
fid=fopen(fullfile(output,'csf.txt'),'w'); assert(fid>=0);
sf=fopen(fullfile(output,'csf_single.txt'),'w'); assert(sf>=0);
count=0;
frequencies=[0,1e-4,1.0001e-4,0.25,0.5,0.9375,1.875,3.75,7.5,15,30,60];
luminances=unique([1e-5,base.csf_lums,0.001,0.01,0.1,1,10,100,1000,1e5]);
for age=[24,45,70]
  for aesl=[false,true]
    p=base; p.age=age; p.do_aesl=aesl;
    for rho=frequencies
      % Paired vector queries keep the reference's low-frequency logical mask
      % the same shape as its output (scalar-rho/vector-lum masks only row1).
      values=hdrvdp_ncsf(repmat(rho,numel(luminances),1),luminances',p);
      single_values=hdrvdp_ncsf(repmat(rho,numel(luminances),1),single(luminances'),p);
      for k=1:numel(luminances)
        fprintf(fid,'%.17g %.17g %d %d %.17g\n',rho,luminances(k),age,aesl,values(k)); count+=1;
        fprintf(sf,'%.17g %.17g %d %d %.17g\n',rho,double(single(luminances(k))),age,aesl,single_values(k));
      end
    end
  end
end
fclose(fid); fprintf('Official v3.0.7 direct nCSF reference: %d cases\n',count);
fclose(sf);
