% Single preprocessing only; no video aggregation or metric prediction.
args=argv();assert(numel(args)==4,'reference-root ref.png test.png fresh-output');
official=canonicalize_file_name(args{1});out=args{4};
assert(~exist(out,'dir'));assert(mkdir(out));
addpath(official);addpath(fullfile(official,'examples'));pkg load image;
assert(strcmp(hash('sha256',fileread(fullfile(official,'hdrvdp3.m'))),'0f9f267f68b042c167ef03f41daaf07388ae192ab30f81b243766863f9a5402a'));
ref=imread(args{2});dis=imread(args{3});
assert(isa(ref,'uint16')&&isa(dis,'uint16'),'This diagnostic requires explicitly decoded 16-bit PNG');
assert(size(ref,3)==3&&size(dis,3)==3);
target=[size(ref,1),size(ref,2)];
result=struct('scope','local RGB48 PNG diagnostic; original author decode precision unresolved',...
 'octave_version',version(),'layout','planar-row-major-little-endian-f32',...
 'width',target(2),'height',target(1),'ppd',hdrvdp_pix_per_deg(64.5,[3840,2160],1.2960),...
 'ambient',200*.005/pi,'peak',2000,'inputs',{{}});
for k=1:2
 if k==1,raw=ref;name='reference';else,raw=dis;name='test';end
 unit=im2single(raw);
 if k==2,unit=min(max(imresize(unit,target,'bicubic'),0),1);end
 luminance=min(pq2lin(unit)+result.ambient,result.peak+result.ambient);
 assert(isa(unit,'single')&&isa(luminance,'single')&&all(isfinite(luminance(:))));
 file=fullfile(out,[name,'.f32']);f=fopen(file,'wb','ieee-le');assert(f>=0);
 count=fwrite(f,permute(luminance,[2,1,3]),'single');fclose(f);assert(count==numel(luminance));
 entry=struct('name',name,'source_sha256',hash('sha256',fileread(args{k+1})),...
 'linear_sha256',hash('sha256',fileread(file)),'unit_class',class(unit),...
 'linear_class',class(luminance),'minimum',double(min(luminance(:))),...
 'maximum',double(max(luminance(:))),'values',numel(luminance));
 result.inputs{end+1}=entry;
 clear raw unit luminance;
end
f=fopen(fullfile(out,'preprocessing.json'),'wt');fprintf(f,'%s\n',jsonencode(result));fclose(f);
disp(jsonencode(result));
