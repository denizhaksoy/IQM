1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
addpath(genpath('C:/HDR-VDP-3_Ref'));
p=hdrvdp_parse_options('quality',{});
lum=logspace(-5,5,2048); grid=log10(lum);
a=hdrvdp_joint_rod_cone_sens(lum,p); r=hdrvdp_rod_sens(lum,p);
c=0.5*interp1(lum,max(a-r,1e-3),min(lum*2,lum(end)));
cones=cumtrapz(grid,c*log(10))*10^p.sensitivity_correction;
rods=cumtrapz(grid,r*log(10))*10^p.sensitivity_correction;
dlmwrite(fullfile(output,'lut.txt'),[grid(:),cones(:),rods(:)],'delimiter',' ','precision','%.17g');
matrix=single([0.65293140835027819,0.34706859164972181,1.0171300679539299]);
inputs=[];
for ch=1:3
  center=single(lum(:))./matrix(ch); bits=typecast(center,'uint32');
  inputs=[inputs; typecast(bits-uint32(1),'single');center;typecast(bits+uint32(1),'single')];
endfor
inputs=unique(inputs);
before=inputs*matrix;
query=log10(clamp(clamp(before,1e-8,1e10),1e-5,1e5));
channels=zeros(size(query),'single');
for ch=1:3
  table=cones; if ch==3, table=rods; endif
  channels(:,ch)=interp1(grid,table,query(:,ch));
endfor
response=(channels(:,1)+channels(:,2))+channels(:,3);
dlmwrite(fullfile(output,'queries.txt'),double([inputs,query,channels,response]),'delimiter',' ','precision','%.17g');
fprintf('JND near-knot cases=%d (three spectral channels; adjacent float inputs)\n',numel(inputs));
