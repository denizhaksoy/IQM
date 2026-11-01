% Independent spectral oracle checked against actual official transform helper.
1;
args=argv();assert(numel(args)==1||numel(args)==2);encoding='rgb-bt.709';if numel(args)==2,encoding=args{2};endif
assert(any(strcmp(encoding,{'rgb-bt.709','rgb-bt.2020','xyz'})));out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));assert(hdrvdp_version()==3.07);
source=fileread(fullfile(official,'utils','hdrvdp_iturgb2native.m'));
source=strrep(source,'function M = hdrvdp_iturgb2native(','function M = iqm_xyz_native(');
anchor='M_native2XYZ = M_native2XYZ / sum(M_native2XYZ(2,:));';assert(numel(strfind(source,anchor))==1);
source=strrep(source,anchor,['global iqm_xyz_trace;iqm_xyz_trace.raw=M_native2XYZ;' char(10) anchor char(10) 'iqm_xyz_trace.normalized=M_native2XYZ;']);
f=fopen(fullfile(out,'iqm_xyz_native.m'),'w');fputs(f,source);fclose(f);addpath(out,'-begin');
[lambda,xyz]=load_spectral_resp(fullfile(official,'data','ciexyz31.csv'));global iqm_xyz_trace;
for display={'led-lcd-srgb','led-lcd-wcg','oled'}
 name=display{1};[~,E]=load_spectral_resp(fullfile(official,'data',['emission_spectra_' name '.csv']));
 stock=hdrvdp_iturgb2native(encoding,E);traced=iqm_xyz_native(encoding,E);
 assert(isequal(typecast(stock(:),'uint64'),typecast(traced(:),'uint64')));
 f=fopen(fullfile(out,[name '_samples.f64']),'wb','ieee-le');fwrite(f,[lambda(:),xyz,E].','double');fclose(f);
 f=fopen(fullfile(out,[name '_matrices.f64']),'wb','ieee-le');fwrite(f,[iqm_xyz_trace.raw,iqm_xyz_trace.normalized,stock],'double');fclose(f);
 fprintf('%s actual official XYZ integrals/normalization/%s matrix captured\n',name,encoding);
endfor
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Native matrices captured;official helper instrumentation bitwise transparent;420-point grid;Double sample rows and column-major matrices.');fclose(f);
f=fopen(fullfile(out,'encoding.txt'),'w');fputs(f,encoding);fclose(f);
