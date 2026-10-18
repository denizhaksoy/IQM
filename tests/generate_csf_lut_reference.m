1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref'; if numel(args)>1, reference=args{2}; endif
addpath(genpath(reference)); assert(hdrvdp_version()==3.07,'Official HDR-VDP-3 v3.0.7 required');
p=hdrvdp_parse_options('quality',{}); p.use_gpu=false;
la=logspace(-5,5,256); grid=log10(la);
fid=fopen(fullfile(output,'csf_lut.txt'),'w'); assert(fid>=0);
queries=unique([grid, (grid(1:end-1)+grid(2:end))/2, double(single(linspace(-5,5,1001)))]);
for age=[24,45,70]
  p.age=age;
  for rho=[15,7.5,3.75,1.875,0.9375]
    values=hdrvdp_ncsf(rho,la',p);
    fprintf(fid,'%d %.17g\n',age,rho);
    fprintf(fid,'%.17g ',values); fprintf(fid,'\n');
    result=interp1(grid,values,queries);
    single_result=interp1(grid,values,single(queries));
    fprintf(fid,'%d\n',numel(queries));
    for k=1:numel(queries)
      fprintf(fid,'%.17g %.17g %.17g\n',queries(k),result(k),single_result(k));
    endfor
  endfor
endfor
fclose(fid);
fprintf('Official CSF LUTs=15, queries per LUT=%d\n',numel(queries));
