% Native-only precision boundary probe, not an implementation/acceptance test.
1;
args=argv();assert(any(numel(args)==[1,2]));stress=numel(args)==2;if stress,assert(strcmp(args{2},'--stress'));endif
out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);fftw('threads',3);fftw('planner','estimate');
original=fileread(fullfile(official,'hdrvdp3.m'));source=strrep(original,'function res = hdrvdp3(','function res = iqm_srgb_native(');
assert(!strcmp(source,original));anchor='metric_par.base_dir = pathstr;';assert(numel(strfind(source,anchor))==1);source=strrep(source,anchor,['metric_par.base_dir = ''' official ''';']);
anchor='RGB = 99*RGB + 1;';assert(numel(strfind(source,anchor))==1);
source=strrep(source,anchor,[anchor char(10) 'global iqm_srgb_display;iqm_srgb_display{end+1}=RGB;']);
assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);source=strrep(source,'hdrvdp_visual_pathway(','iqm_srgb_visual(');
assert(numel(strfind(source,'hdrvdp_colorspace_transform('))==2);source=strrep(source,'hdrvdp_colorspace_transform(','iqm_srgb_transform(');
f=fopen(fullfile(out,'iqm_srgb_native.m'),'w');fputs(f,source);fclose(f);
f=fopen(fullfile(out,'iqm_srgb_transform.m'),'w');fputs(f,['function result=iqm_srgb_transform(input,matrix)' char(10) 'result=hdrvdp_colorspace_transform(input,matrix);global iqm_srgb_transform_trace;iqm_srgb_transform_trace{end+1}=struct(''input'',input,''matrix'',matrix,''output'',result);' char(10) 'end']);fclose(f);
f=fopen(fullfile(out,'iqm_srgb_visual.m'),'w');fputs(f,['function varargout=iqm_srgb_visual(varargin)' char(10) 'global iqm_srgb_visual_trace;iqm_srgb_visual_trace{end+1}=varargin{1};[varargout{1:nargout}]=hdrvdp_visual_pathway(varargin{:});' char(10) 'end']);fclose(f);addpath(out,'-begin');
[x,y]=meshgrid(0:95,0:95);reference=cat(3,single(x/95),single(y/95),single(.5+.49*sin(x/7).*cos(y/9)));
reference(1:4,1:4,:)=single(.04044);reference(5:8,1:4,:)=single(.04045);reference(9:12,1:4,:)=single(.04046);
test=reference;test(40:65,45:75,1)*=single(.9);test(10:25,60:80,3)=min(single(1),test(10:25,60:80,3)+single(.01));
if stress
 % Deterministic varied encoded codes, with exact endpoints, primaries and
 % adjacent Single values around the native Double branch threshold.
 codes=single(mod((0:27647)*7919,1048576)/1048575);
 reference=reshape(codes,[96,96,3]);test=reshape(codes(end:-1:1),[96,96,3]);
 threshold=typecast(single(.04045),'uint32');edges=[single(0),single(1),typecast(threshold-uint32(1),'single'),typecast(threshold,'single'),typecast(threshold+uint32(1),'single')];
 for c=1:3
  reference(1:5,1,c)=edges';test(1:5,1,c)=edges(end:-1:1)';
  reference(10:15,10:15,c)=single(c==1);test(10:15,10:15,c)=single(c==2);
  reference(20:25,20:25,c)=single(c==3);test(20:25,20:25,c)=single(0);
 endfor
endif
global iqm_srgb_display iqm_srgb_transform_trace iqm_srgb_visual_trace;records={};
for display={'led-lcd-srgb','led-lcd-wcg','oled'}
 name=display{1};options={'use_gpu',false,'quiet',true,'age',24,'rgb_display',name};
 fftw('dwisdom','');log=evalc("stock=hdrvdp3('quality',test,reference,'srgb-display',30,options);");
 iqm_srgb_display={};iqm_srgb_transform_trace={};iqm_srgb_visual_trace={};fftw('dwisdom','');log=evalc("traced=iqm_srgb_native('quality',test,reference,'srgb-display',30,options);");
 assert(isa(stock.Q,'double')&&isa(stock.Q_JOD,'double'));
 assert(isequal(typecast(stock.Q,'uint64'),typecast(traced.Q,'uint64'))&&isequal(typecast(stock.Q_JOD,'uint64'),typecast(traced.Q_JOD,'uint64')));
 assert(numel(iqm_srgb_display)==2&&numel(iqm_srgb_transform_trace)==2&&numel(iqm_srgb_visual_trace)==2);
 mismatches=0;
 for side=1:2
  eotf=iqm_srgb_display{3-side};t=iqm_srgb_transform_trace{3-side};v=iqm_srgb_visual_trace{side};
  assert(isa(eotf,'double')&&isa(t.input,'double')&&isa(t.output,'double')&&isa(v,'double'));
  converted=double(single(v));mismatches+=sum(typecast(v(:),'uint64')!=typecast(converted(:),'uint64'));
  prefix=fullfile(out,[name '_' num2str(side)]);
  for field={'eotf','v'},f=fopen([prefix '_' field{1} '.f64'],'wb','ieee-le');fwrite(f,eval(field{1}),'double');fclose(f);endfor
  input=reference;if side==2,input=test;endif
  f=fopen([prefix '_input.f32'],'wb','ieee-le');fwrite(f,input,'single');fclose(f);
  f=fopen([prefix '_matrix.f64'],'wb','ieee-le');fwrite(f,t.matrix,'double');fclose(f);
  f=fopen([prefix '_raw.f64'],'wb','ieee-le');fwrite(f,t.output,'double');fclose(f);
 endfor
 records{end+1}=struct('display',name,'source_type','single','display_output_type','double','matrix_output_type','double','visual_input_type','double','Q_type',class(stock.Q),'Q',stock.Q,'Q_JOD',stock.Q_JOD,'Float32_roundtrip_bit_changes',mismatches,'values',2*96*96*3);
 fprintf('%s nativeDouble path;Float32roundtrip changes%d/55296 values\n',name,mismatches);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'stress_fixture',stress,'scope','Native precision probe only,not production sRGB or GPU acceptance','encoding','srgb-display','parameters','Single encoded96x96;30ppd/age24;Double FFTW3threads3/estimate/dwisdom cleared','records',{records})));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Stock/instrumented nativeDouble score bits equal;EOTF/matrix/visual types measured. Float32 rounding evidence only;no production sRGB acceptance.');fclose(f);
