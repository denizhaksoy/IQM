1;
% Additional native lookup checks: adjacent double knots and explicit clipping.
args=argv(); output=args{1}; assert(!exist(output,'dir')); assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref'; if numel(args)>1, reference=args{2}; endif
addpath(genpath(reference)); assert(hdrvdp_version()==3.07);
p=hdrvdp_parse_options('quality',{}); p.use_gpu=false;
la=logspace(-5,5,256); grid=log10(la);
bits=typecast(grid,'uint64');
neighbors=[typecast(bits-uint64(1),'double'),typecast(bits+uint64(1),'double')];
rand('state',71806);
queries=unique([grid,neighbors,-5.5,5.5,-5-2^-20,5+2^-20,-5+10*rand(1,2048)]);
f=fopen(fullfile(output,'csf_lut.txt'),'w'); assert(f>=0);
for age=[24,45,70]
  p.age=age;
  for rho=[15,7.5,3.75,1.875,0.9375]
    values=hdrvdp_ncsf(rho,la',p);
    fprintf(f,'%d %.17g\n',age,rho); fprintf(f,'%.17g\n',values);
    expected=interp1(grid,values,clamp(queries,grid(1),grid(end)));
    single_expected=interp1(grid,values,clamp(single(queries),grid(1),grid(end)));
    fprintf(f,'%d\n',numel(queries));
    for k=1:numel(queries)
      fprintf(f,'%.17g %.17g %.17g\n',queries(k),expected(k),single_expected(k));
    endfor
  endfor
endfor
fclose(f);
fprintf('Native LUT boundary/random fixture:15 LUTs x%d queries\n',numel(queries));
