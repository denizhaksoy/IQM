% Independent official rectangular full-quality reference and PFM inputs.
1;
function a=read_map(path)
 f=fopen(path,'r');assert(f>=0);dims=fscanf(f,'%d',2);v=fscanf(f,'%f');fclose(f);
 assert(numel(v)==prod(dims));a=single(reshape(v,dims(2),dims(1)).');
endfunction
function write_pfm(path,a)
 f=fopen(path,'wb','ieee-le');assert(f>=0);fprintf(f,'Pf\n%d %d\n-1.0\n',columns(a),rows(a));
 fwrite(f,flipud(a).','single');fclose(f);
endfunction
args=argv();source=args{1};out=args{2};transform='localized';
if numel(args)>2,transform=args{3};endif
assert(any(strcmp(transform,{'localized','identity'})));
assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);pkg load image;pkg load statistics;
fftw('threads',3);fftw('swisdom','');
reference=read_map(fullfile(source,'ref_luminance.txt'));[h,w]=size(reference);
assert((h==64&&w==128)||(h==128&&w==64));
test=reference;ys=(floor(h*.25)+1):floor(h*.65);xs=(floor(w*.55)+1):floor(w*.85);
if strcmp(transform,'localized'),test(ys,xs)=reference(ys,xs)*single(1.4)+single(.25);endif
assert(all(isfinite(reference(:)))&&all(reference(:)>0)&&all(isfinite(test(:)))&&all(test(:)>0));
write_pfm(fullfile(out,'reference.pfm'),reference);write_pfm(fullfile(out,'test.pfm'),test);
result=hdrvdp3('quality',test,reference,'luminance',30,{'use_gpu',false,'age',24,'quiet',true});
assert(isfinite(result.Q)&&isfinite(result.Q_JOD));save(fullfile(out,'official_result.mat'),'result');
f=fopen(fullfile(out,'scores.txt'),'w');assert(f>=0);fprintf(f,'%.17g %.17g\n',result.Q,result.Q_JOD);fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('scope','Independent official rectangular quality reference','version',hdrvdp_version(),...
 'reference',which('hdrvdp3'),'source',source,'shape',[h,w],'precision','single','ppd',30,'age',24,...
 'threads_requested',3,'pfm_layout','bottom-up,little-endian Float32',...
 'transform',transform,'localized_formula','reference*single1.4+single.25','Q',result.Q,'Q_JOD',result.Q_JOD)));fclose(f);
fprintf('Official rectangular%dx%d Q=%.17g Q_JOD=%.17g\n',h,w,result.Q,result.Q_JOD);
