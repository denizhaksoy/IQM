% Actual official linear709 matrix/projection/clamp capture, no production use.
1;
args=argv();assert(any(numel(args)==[1,3,5]));encoding='rgb-bt.709';input_dir='';
for i=2:2:numel(args)
 if strcmp(args{i},'--rgb-input'),assert(isempty(input_dir));input_dir=args{i+1};
 elseif strcmp(args{i},'--encoding'),encoding=args{i+1};
 else,error('Unknown reference option');endif
endfor
assert(any(strcmp(encoding,{'rgb-bt.709','rgb-bt.2020','xyz'})));external=!isempty(input_dir);
out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);fftw('threads',3);fftw('planner','estimate');
original=fileread(fullfile(official,'hdrvdp3.m'));source=strrep(original,'function res = hdrvdp3(','function res = iqm_709_native(');
assert(!strcmp(original,source));assert(numel(strfind(source,'metric_par.base_dir = pathstr;'))==1);
source=strrep(source,'metric_par.base_dir = pathstr;',['metric_par.base_dir = ''' official ''';']);
anchor='Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();';assert(numel(strfind(source,anchor))==1);
source=strrep(source,anchor,[anchor char(10) 'global iqm_709_bands;iqm_709_bands{b}=struct(''error'',minkowski_sum(D_p(:),.8),''accumulated'',Q_err);']);
assert(numel(strfind(source,'hdrvdp_colorspace_transform('))==2);source=strrep(source,'hdrvdp_colorspace_transform(','iqm_709_transform(');
assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);source=strrep(source,'hdrvdp_visual_pathway(','iqm_709_visual(');
f=fopen(fullfile(out,'iqm_709_native.m'),'w');fputs(f,source);fclose(f);
f=fopen(fullfile(out,'iqm_709_transform.m'),'w');fputs(f,['function result=iqm_709_transform(input,matrix)' char(10) 'result=hdrvdp_colorspace_transform(input,matrix);global iqm_709_trace;iqm_709_trace{end+1}=struct(''input'',input,''matrix'',matrix,''raw'',result);' char(10) 'end']);fclose(f);
f=fopen(fullfile(out,'iqm_709_visual.m'),'w');fputs(f,['function varargout=iqm_709_visual(varargin)' char(10) 'global iqm_709_clamped iqm_709_response;iqm_709_clamped{end+1}=varargin{1};[varargout{1:nargout}]=hdrvdp_visual_pathway(varargin{:});iqm_709_response{end+1}=struct(''adaptation'',varargout{2},''P'',varargout{4});' char(10) 'end']);fclose(f);addpath(out,'-begin');
[x,y]=meshgrid(0:95,0:95);reference=cat(3,single(.01+300*(x/95).^2),single(.02+80*(y/95).^3),single(.001+20*(1+sin(x/7).*cos(y/9))));
reference(1:16,1:16,:)=0;reference(30:45,20:35,2:3)=0;
test=reference;test(40:65,45:75,1)*=single(.8);test(10:25,60:80,3)+=single(3);test(5:8,5:8,3)=single(.25);
if external
 f=fopen(fullfile(input_dir,'reference_rgb.f32'),'rb','ieee-le');assert(f>=0);v=fread(f,Inf,'*single');fclose(f);assert(numel(v)==3*96*96);reference=reshape(v,96,96,3);
 f=fopen(fullfile(input_dir,'test_rgb.f32'),'rb','ieee-le');assert(f>=0);v=fread(f,Inf,'*single');fclose(f);assert(numel(v)==3*96*96);test=reshape(v,96,96,3);
 assert(all(isfinite(reference(:)))&&all(reference(:)>=0)&&all(isfinite(test(:)))&&all(test(:)>=0));
endif
global iqm_709_trace iqm_709_clamped iqm_709_response iqm_709_bands;records={};
for display={'led-lcd-srgb','led-lcd-wcg','oled'}
 name=display{1};options={'use_gpu',false,'quiet',true,'age',24,'rgb_display',name};
 fftw('swisdom','');log=evalc("stock=hdrvdp3('quality',test,reference,encoding,30,options);");
 iqm_709_trace={};iqm_709_clamped={};iqm_709_response={};iqm_709_bands={};fftw('swisdom','');log=evalc("traced=iqm_709_native('quality',test,reference,encoding,30,options);");
 assert(isequal(typecast(stock.Q,'uint32'),typecast(traced.Q,'uint32'))&&isequal(typecast(stock.Q_JOD,'uint32'),typecast(traced.Q_JOD,'uint32')));
 assert(numel(iqm_709_trace)==2&&numel(iqm_709_clamped)==2);
 clamped=0;
 for side=1:2
  t=iqm_709_trace{3-side};post=iqm_709_clamped{side};assert(isa(t.input,'single')&&isa(t.raw,'single')&&isa(post,'single')&&isa(t.matrix,'double'));
  assert(isequal(typecast(max(t.raw(:),single(1e-6)),'uint32'),typecast(post(:),'uint32')));clamped+=sum(t.raw(:)<single(1e-6));
  prefix=fullfile(out,[name '_' num2str(side)]);
  for field={'input','raw'}
   f=fopen([prefix '_' field{1} '.f32'],'wb','ieee-le');fwrite(f,t.(field{1}),'single');fclose(f);
  endfor
  f=fopen([prefix '_clamped.f32'],'wb','ieee-le');fwrite(f,post,'single');fclose(f);
  f=fopen([prefix '_matrix.f64'],'wb','ieee-le');fwrite(f,t.matrix,'double');fclose(f);
  for field={'adaptation','P'},v=iqm_709_response{side}.(field{1});assert(isa(v,'single'));f=fopen([prefix '_' field{1} '.f32'],'wb','ieee-le');fwrite(f,v,'single');fclose(f);endfor
 endfor
 f=fopen(fullfile(out,[name '_quality.f32']),'wb','ieee-le');fwrite(f,[stock.Q,stock.Q_JOD],'single');fclose(f);
 assert(numel(iqm_709_bands)==5);errors=single(zeros(5,1));acc=0;
 for b=1:5,errors(b)=iqm_709_bands{b}.error;acc=acc+errors(b)/5;assert(isequal(acc,iqm_709_bands{b}.accumulated));endfor
 f=fopen(fullfile(out,[name '_errors.f32']),'wb','ieee-le');fwrite(f,[errors;acc],'single');fclose(f);
 records{end+1}=struct('display',name,'clamped_values',clamped,'Q',stock.Q,'Q_JOD',stock.Q_JOD);
 fprintf('%s linear709 stock/traced bits exact;Single projection;clamped%d\n',name,clamped);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'encoding',encoding,'scope','Native conversion characterization only,not production709 or GPU quality acceptance','layout','column-major planar;3x3matrix column-major','parameters','Float32 input/projection,Float64matrix;96x96/30ppd/age24;native symmetric MTF/replicate adaptation/DC unchanged','records',{records})));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Three linear709 native pairs;stock/traced score bits exact;actual pre/post1e-6 gamut clamp captured.');fclose(f);
