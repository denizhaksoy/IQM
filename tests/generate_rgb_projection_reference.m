% Test-only captures at the native optical -> LMSR boundary.
1;
args=argv();assert(any(numel(args)==[1,2,4,6]));extended=numel(args)>=2;pattern='ramps';
if numel(args)>=4,assert(strcmp(args{3},'--pattern'));pattern=args{4};endif
assert(any(strcmp(pattern,{'ramps','identity','constant','primaries','impulse','external'})));
if numel(args)==6,assert(strcmp(args{5},'--rgb-input')&&strcmp(pattern,'external'));endif
if strcmp(pattern,'external'),assert(numel(args)==6);endif
if extended,assert(strcmp(args{2},'--jnd'));endif
out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);fftw('threads',3);fftw('planner','estimate');
original=fileread(fullfile(official,'hdrvdp3.m'));
source=strrep(original,'function res = hdrvdp3(','function res = iqm_rgb_projection_native(');
assert(!strcmp(source,original));assert(numel(strfind(source,'metric_par.base_dir = pathstr;'))==1);
source=strrep(source,'metric_par.base_dir = pathstr;',['metric_par.base_dir = ''' official ''';']);
assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);
source=strrep(source,'hdrvdp_visual_pathway(','iqm_rgb_projection_visual(');
if extended
 anchor='diff_mask = any((abs( test-reference ) ./ reference) > 0.001, 3);';
 assert(numel(strfind(source,anchor))==1);
 source=strrep(source,anchor,[anchor char(10) 'global iqm_rgb_comparison_mask;iqm_rgb_comparison_mask=diff_mask;']);
 anchor='Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();';
 assert(numel(strfind(source,anchor))==1);
 source=strrep(source,anchor,[anchor char(10) 'global iqm_rgb_quality_bands;iqm_rgb_quality_bands{b}=struct(''masked'',D_p,''error'',minkowski_sum(D_p(:),.8),''accumulated'',Q_err);']);
endif
visual=fileread(fullfile(official,'hdrvdp_visual_pathway.m'));
visual=strrep(visual,' = hdrvdp_visual_pathway(',' = iqm_rgb_projection_visual(');
anchor='R_LMSR = clamp( R_LMSR, 1e-8, 1e10 ); % To avoid negative numbers';
assert(numel(strfind(visual,anchor))==1);
capture=['global iqm_rgb_projection_trace; iqm_rgb_projection_trace{end+1}=struct(''input'',img,''optical'',L_O,''matrix'',M_img_lmsr,''response'',R_LMSR);'];
visual=strrep(visual,anchor,[capture char(10) anchor]);
if extended
 anchor='L_adapt = hdrvdp_local_adapt( R_LMSR(:,:,1) + R_LMSR(:,:,2), metric_par.pix_per_deg );';
 assert(numel(strfind(visual,anchor))==1);
 visual=strrep(visual,anchor,[anchor char(10) 'iqm_rgb_projection_trace{end}.clamped=R_LMSR;iqm_rgb_projection_trace{end}.adaptation_input=R_LMSR(:,:,1)+R_LMSR(:,:,2);iqm_rgb_projection_trace{end}.adaptation=L_adapt;']);
 anchor='P_LMR = zeros_like( R_LMSR );';assert(numel(strfind(visual,anchor))==1);
 visual=strrep(visual,anchor,[anchor char(10) 'iqm_rgb_projection_trace{end}.post_pupil=R_LMSR;iqm_rgb_projection_trace{end}.pupil_reduction=lum_reduction;']);
 anchor='P = P_C + P_R;';assert(numel(strfind(visual,anchor))==1);
 visual=strrep(visual,anchor,[anchor char(10) 'iqm_rgb_projection_trace{end}.jnd=P_LMR;iqm_rgb_projection_trace{end}.P=P;']);
endif
f=fopen(fullfile(out,'iqm_rgb_projection_native.m'),'w');fputs(f,source);fclose(f);
f=fopen(fullfile(out,'iqm_rgb_projection_visual.m'),'w');fputs(f,visual);fclose(f);addpath(out,'-begin');
[x,y]=meshgrid(0:95,0:95);
reference=cat(3,single(.01+300*(x/95).^2),single(.02+80*(y/95).^3),single(.001+20*(1+sin(x/7).*cos(y/9))));
reference(1:16,1:16,:)=0;reference(30:45,20:35,2:3)=0;
test=reference;test(40:65,45:75,1)=test(40:65,45:75,1)*single(.8);test(10:25,60:80,3)=test(10:25,60:80,3)+single(3);
% Optional full trace also tests positive/zero alongside unchanged zero/zero.
if extended,test(5:8,5:8,3)=single(.25);endif
switch pattern
 case 'identity'
  test=reference;
 case 'constant'
  reference=cat(3,single(.125)*ones(96,96,'single'),single(10)*ones(96,96,'single'),single(1000)*ones(96,96,'single'));
  test=reference;test(:,:,2)=test(:,:,2)*single(1.01);
 case 'primaries'
  reference=zeros(96,96,3,'single');reference(:,1:32,1)=single(100);reference(:,33:64,2)=single(25);reference(:,65:96,3)=single(5);
  test=reference;test(25:65,40:55,2)=test(25:65,40:55,2)*single(.85);test(5:8,5:8,3)=single(.25);
 case 'impulse'
  reference=single(.01)*ones(96,96,3,'single');reference(20,30,1)=single(10000);reference(70,60,2)=single(1000);reference(45,75,3)=single(100);
  test=reference;test(20,30,1)=single(7500);test(71,60,2)=single(30);test(45,75,3)=single(80);
 case 'external'
  f=fopen(fullfile(args{6},'reference_rgb.f32'),'rb','ieee-le');assert(f>=0);v=fread(f,Inf,'*single');fclose(f);assert(numel(v)==3*96*96);reference=reshape(v,96,96,3);
  f=fopen(fullfile(args{6},'test_rgb.f32'),'rb','ieee-le');assert(f>=0);v=fread(f,Inf,'*single');fclose(f);assert(numel(v)==3*96*96);test=reshape(v,96,96,3);
  assert(all(isfinite(reference(:)))&&all(reference(:)>=0)&&all(isfinite(test(:)))&&all(test(:)>=0));
endswitch
global iqm_rgb_projection_trace;records={};
for display={'led-lcd-srgb','led-lcd-wcg','oled'}
 name=display{1};options={'use_gpu',false,'quiet',true,'age',24,'rgb_display',name};
 fftw('swisdom','');stocklog=evalc("stock=hdrvdp3('quality',test,reference,'rgb-native',30,options);");
 iqm_rgb_projection_trace={};fftw('swisdom','');tracelog=evalc("traced=iqm_rgb_projection_native('quality',test,reference,'rgb-native',30,options);");
 assert(isequal(typecast(stock.Q,'uint32'),typecast(traced.Q,'uint32')));
 assert(isequal(typecast(stock.Q_JOD,'uint32'),typecast(traced.Q_JOD,'uint32')));
 assert(numel(iqm_rgb_projection_trace)==2);
 f=fopen(fullfile(out,[name '_quality.f32']),'wb','ieee-le');fwrite(f,[stock.Q,stock.Q_JOD],'single');fclose(f);
 if extended
  global iqm_rgb_comparison_mask;
  global iqm_rgb_quality_bands;assert(numel(iqm_rgb_quality_bands)==5);errors=single(zeros(5,1));acc=0;
  for band=1:5
   v=iqm_rgb_quality_bands{band};assert(isa(v.masked,'single'));errors(band)=v.error;acc=acc+errors(band)/5;assert(isequal(acc,v.accumulated));
   f=fopen(fullfile(out,[name '_band' num2str(band) '_masked.f32']),'wb','ieee-le');fwrite(f,v.masked,'single');fclose(f);
  endfor
  f=fopen(fullfile(out,[name '_errors.f32']),'wb','ieee-le');fwrite(f,[errors;acc],'single');fclose(f);
  independent=any((abs(test-reference)./reference)>single(.001),3);
  assert(isequal(independent,iqm_rgb_comparison_mask));
  if strcmp(pattern,'ramps')||strcmp(pattern,'primaries'),assert(all(iqm_rgb_comparison_mask(5:8,5:8)(:))&&!iqm_rgb_comparison_mask(1,1));endif
  if strcmp(pattern,'identity'),assert(!any(iqm_rgb_comparison_mask(:))&&stock.Q==10&&stock.Q_JOD==10);endif
  for scale=[1,2,4,8]
   mask=imresize(double(iqm_rgb_comparison_mask),[96/scale,96/scale]);
   f=fopen(fullfile(out,[name '_mask' num2str(scale) '.f64']),'wb','ieee-le');fwrite(f,mask,'double');fclose(f);
  endfor
  f=fopen(fullfile(out,[name '_mask.u32']),'wb','ieee-le');fwrite(f,iqm_rgb_comparison_mask,'uint32');fclose(f);
 endif
 for side=1:2
  t=iqm_rgb_projection_trace{side};assert(isa(t.optical,'single')&&isa(t.response,'single')&&isa(t.matrix,'double'));
  prefix=fullfile(out,[name '_' num2str(side)]);
  f=fopen([prefix '_input.f32'],'wb','ieee-le');fwrite(f,t.input,'single');fclose(f);
  f=fopen([prefix '_optical.f32'],'wb','ieee-le');fwrite(f,t.optical,'single');fclose(f);
  f=fopen([prefix '_matrix.f64'],'wb','ieee-le');fwrite(f,t.matrix,'double');fclose(f);
  f=fopen([prefix '_response.f32'],'wb','ieee-le');fwrite(f,t.response,'single');fclose(f);
  if extended
   for field={'clamped','adaptation_input','adaptation','post_pupil','jnd','P'}
    f=fopen([prefix '_' field{1} '.f32'],'wb','ieee-le');fwrite(f,t.(field{1}),'single');fclose(f);
   endfor
   f=fopen([prefix '_pupil.txt'],'w');fprintf(f,'%.17g\n',t.pupil_reduction);fclose(f);
   assert(all(isfinite(t.P(:)))&&all(isfinite(t.adaptation(:)))&&all(t.adaptation(:)>0));
  endif
 endfor
 records{end+1}=struct('display',name,'Q',stock.Q,'Q_JOD',stock.Q_JOD,'height',96,'width',96,'age',24,'ppd',30);
 fprintf('%s nonidentity stock/traced scores exact; two RGB optical/LMSR captures\n',name);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('version',3.07,'pattern',pattern,'encoding','rgb-native','layout','column-major planar; matrix3x4 column-major','precision','input/optical/response Single; matrix Double; pre-response-clamp','padding','native symmetric MTF; no input zero clamp','dc','native MTF DC retained','records',{records})));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fprintf(f,'3 nonidentity RGB96 pairs;6 optical/LMSR captures;stock/traced score bits exact. Not GPU acceptance.');fclose(f);
if extended
 f=fopen(fullfile(out,'JND_SCOPE.txt'),'w');fprintf(f,'Additional captures:clamped LMSR,L+M,adaptation,post-pupil LMSR,4-plane JND (unused plane4 zero),P. Float32 column-major. Pupil scalar text preserves source precision. Native captures only,not GPU JND acceptance.');fclose(f);
endif
