% Real visual-pathway residuals: reconstruct before DC, retain official order.
1;
function write_words(path,words)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'1 %d\n',numel(words));fprintf(f,'%u\n',words);fclose(f);
endfunction
function z=pack(a)
 z=zeros(240,1,'uint32');z(1:2:end)=typecast(a(:),'uint32');
endfunction
function write_floats(path,words)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'1 %d\n',numel(words));
 fprintf(f,'%.17g\n',typecast(words,'single'));fclose(f);
endfunction
function e=spectral(a)
 fftw('swisdom','');s=fft2(a);m=abs(s);m(1,1)=0;[v,index]=max(m(:));assert(isfinite(v));
 row=reshape(s.',[],1);e=zeros(361,1,'uint32');
 e(1:2:240)=typecast(real(row),'uint32');e(2:2:240)=typecast(imag(row),'uint32');
 e(241:360)=typecast(reshape(m.',[],1),'uint32');e(361)=typecast(single(index-1),'uint32');
endfunction
args=argv();assert(numel(args)==2);out=args{1};source=args{2};
assert(!exist(out,'dir'));assert(mkdir(out));addpath(genpath('C:/HDR-VDP-3_Ref'));
assert(hdrvdp_version()==3.07);pkg load image;fftw('threads',3);fftw('planner','estimate');
meta=jsondecode(fileread(fullfile(source,'metadata.json')));assert(meta.version==3.07);
pipeline=fopen(fullfile(out,'pipeline_manifest.txt'),'w');difference=fopen(fullfile(out,'difference_manifest.txt'),'w');
combined=fopen(fullfile(out,'combined_manifest.txt'),'w');
assert(pipeline>=0&&difference>=0&&combined>=0);records={};
for k=1:numel(meta.records)
 shape=meta.records(k).shape;tag=sprintf('h%d_w%d',shape(1),shape(2));
 loaded=load(fullfile(source,tag,'trace.mat'));t=loaded.trace;b=rows(t.bands);
 h=rows(t.bands{b,1}.reference);w=columns(t.bands{b,1}.reference);
 input=[];means=[];packed=[];expected=[];ranges=[];
 for label={'reference','test'}
  n=label{1};p=hdrvdp_spyr('sp0Filters');p=p.decompose(t.(['P_' n]),30);
  raw=p.get_band(p.band_count(),1);assert(isa(raw,'single')&&numel(raw)==120);
  m=mean(raw(:));assert(isequal(typecast(m,'uint32'),typecast(single(sum(double(raw(:)))/120),'uint32')));
  centered=raw-m;actual=t.bands{b,1}.(n);
  assert(isequal(typecast(centered(:),'uint32'),typecast(actual(:),'uint32')));
  nz=abs(double(raw(raw!=0)));ex=floor(log2(nz));assert(isempty(ex)||(min(ex)>=-90&&max(ex)<=100&&max(ex)-min(ex)<=16));
  ranges=[ranges;min(ex),max(ex)];
  input=[input;typecast(reshape(raw.',[],1),'uint32')];
  means=[means;typecast([m;reshape(centered.',[],1)],'uint32')];
  packed=[packed;pack(centered)];expected=[expected;spectral(centered)];
 endfor
 folder=fullfile(out,tag);assert(mkdir(folder));names={'input','mean','pack','expected'};values={input,means,packed,expected};paths={};
 for i=1:4,paths{i}=strrep(fullfile(folder,[names{i} '.txt']),'\','/');write_words(paths{i},values{i});endfor
 fprintf(pipeline,'%s %d %d "%s" "%s" "%s" "%s"\n',tag,h,w,paths{:});
 d=t.bands{b,1}.test-t.bands{b,1}.reference;
 assert(isequal(typecast(d(:),'uint32'),typecast(t.base.difference(:),'uint32')));
 e=spectral(d);assert(typecast(e(361),'single')==single(t.base.index-1));
 % Raw FFT DC is preserved in component output; official trace already zeroed it.
 traced=t.base.spectrum;s=fft2(d);s(1,1)=0;assert(isequal(s,traced));
 in=strrep(fullfile(folder,'difference_input.txt'),'\','/');ep=strrep(fullfile(folder,'difference_expected.txt'),'\','/');
 write_floats(in,pack(d));write_floats(ep,e);fprintf(difference,'%s %d %d "%s" "%s"\n',tag,h,w,in,ep);
 dp=strrep(fullfile(folder,'difference_pack_words.txt'),'\','/');de=strrep(fullfile(folder,'difference_expected_words.txt'),'\','/');
 write_words(dp,pack(d));write_words(de,e);
 fprintf(combined,'%s %d %d "%s" "%s" "%s" "%s"\n',tag,h,w,paths{1},paths{2},dp,de);
 records{end+1}=struct('shape',shape,'residual_shape',[h,w],'raw_exponents',ranges,'centered_bit_identity',true,'selection_identity',true);
endfor
fclose(pipeline);fclose(difference);fclose(combined);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'source',source,'records',{records},...
 'precision','single; native double accumulation mean; once-rounded single DC subtraction',...
 'layout','mean input row-major; FFT input column-major interleaved; output row-major',...
 'padding','none in residual; official upstream visual pathway','normalization','mean/120 only; forward unnormalized',...
 'DC','Center reference/test separately, then subtract; do not recenter difference; zero magnitude DC only')));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Four reconstructed raw base bands reproduce centered trace bitwise; two base differences reproduce traced FFT and index.');fclose(f);
