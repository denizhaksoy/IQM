% Independent official full128x128/30ppd/age24 quality reference and PFM inputs.
1;
function image=read_map(path)
 f=fopen(path,'r');assert(f>=0);dims=fscanf(f,'%d',2);v=fscanf(f,'%f');fclose(f);
 assert(isequal(dims(:)',[128,128])&&numel(v)==16384);image=single(reshape(v,128,128).');
endfunction
function write_pfm(path,image)
 f=fopen(path,'wb','ieee-le');assert(f>=0);fprintf(f,'Pf\n128 128\n-1.0\n');fwrite(f,flipud(image).','single');fclose(f);
endfunction
args=argv();source=args{1};out=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;pkg load statistics;fftw('threads',3);fftw('swisdom','');
reference=read_map(fullfile(source,'ref_luminance.txt'));test=read_map(fullfile(source,'test_luminance.txt'));
transform='source';if numel(args)>2
 transform=args{3};
 switch transform
 case 'localized'
  test=reference;test(17:43,79:111)=reference(17:43,79:111)*single(1.4)+single(.25);
 case 'impulse'
  reference=ones(128,128,'single')*single(10);test=reference;
  test(1,1)=single(80);test(65,64)=single(120);test(128,128)=single(60);
 case 'competing_axes'
  % Equal input-axis amplitudes with a one-ULP perturbation. Native residual
  % selection must be measured; this is not an asserted residual near-tie.
  [x,y]=meshgrid(0:127,0:127);
  reference=ones(128,128,'single')*single(10);
  a=single(2);b=typecast(typecast(a,'uint32')+uint32(1),'single');
  test=reference+a*single(cos(2*pi*x/64))+b*single(cos(2*pi*y/64));
 otherwise
  error('Unknown128 transform:%s',transform);
 endswitch
endif
assert(all(isfinite(reference(:)))&&all(reference(:)>0)&&all(isfinite(test(:)))&&all(test(:)>0));
write_pfm(fullfile(out,'reference.pfm'),reference);write_pfm(fullfile(out,'test.pfm'),test);
result=hdrvdp3('quality',test,reference,'luminance',30,{'use_gpu',false,'age',24,'quiet',true});
assert(isfinite(result.Q)&&isfinite(result.Q_JOD));save(fullfile(out,'official_result.mat'),'result');
% The official result exposes Q/Q_JOD; pooled error is checked independently
% by native stage validators after GPU execution,without inverting rounded Q.
f=fopen(fullfile(out,'scores.txt'),'w');fprintf(f,'%.17g %.17g\n',result.Q,result.Q_JOD);fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Independent native full128 quality reference','version',hdrvdp_version(),'function',which('hdrvdp3'),'input_precision','single','input_source',source,'transform',transform,'pfm_layout','bottom-up rows,little-endian float32','ppd',30,'age',24,'threads',fftw('threads'),'Q',result.Q,'Q_JOD',result.Q_JOD)));fclose(f);
fprintf('Official128 Q=%.17g Q_JOD=%.17g\n',result.Q,result.Q_JOD);
