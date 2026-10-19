% Isolated pupil reference; this does not validate age-dependent spectra/JND.
1;
args=argv();output=args{1};assert(!exist(output,'dir'));assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref';if numel(args)>1,reference=args{2};endif
addpath(genpath(reference));pkg load statistics;
assert(hdrvdp_version()==3.07,'Official HDR-VDP-3 v3.0.7 required');
rand('state',81349);
ages=[0,15,19.999,20,24,28,45,60,60.001,75,83,83.001,100];
f=fopen(fullfile(output,'pupil_age.txt'),'w');assert(f>=0);count=0;
for side=[32,64]
  [x,y]=meshgrid(0:side-1,0:side-1);
  maps={repmat(single(.001),side),repmat(single(10),side),...
        repmat(single(1e4),side),single(10.^(-3+7*(3*x+2*y)/(5*(side-1)))),...
        single(10.^(-3+7*rand(side)))};
  for ppd=[30,60]
    area=side*side/ppd^2;
    for pattern=1:numel(maps)
      image=maps{pattern};geo=geomean(image(:));
      for age=ages
        dref=pupil_d_unified(geo,area,28);dage=pupil_d_unified(geo,area,age);
        expected=[geo,dref,dage,dage.^2./dref.^2];assert(isa(expected,'single'));
        fprintf(f,'%d %d %.17g %.17g\n',side,side,area,age);
        fprintf(f,'%.9g\n',image'); % Row-major transport; native reduction stays column-major.
        fprintf(f,'%.9g\n',expected);count+=1;
      endfor
    endfor
  endfor
endfor
fclose(f);
meta=struct('version',hdrvdp_version(),'ages',ages,'sizes',[32,64],...
  'ppd',[30,60],'patterns',5,'cases',count,'trace_values',4*count,...
  'precision','native single','layout','row-major file;column-major reduction',...
  'scope','isolated pupil only;no age-dependent spectral/JND/full-metric claim');
f=fopen(fullfile(output,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(meta));fclose(f);
fprintf('Pupil age reference: %d cases, %d single trace values\n',count,4*count);
