% RGB spectral oracle; actual official matrices captured before BLAS projection.
1;
function s=insert_once(s,anchor,extra)
 assert(numel(strfind(s,anchor))==1);s=strrep(s,anchor,[anchor char(10) extra]);
endfunction
function same(a,b)
 assert(isequal(size(a),size(b))&&isa(a,'double')&&isa(b,'double'));
 assert(isequal(typecast(a(:),'uint64'),typecast(b(:),'uint64')));
endfunction
args=argv();assert(numel(args)==1);out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
official='C:/HDR-VDP-3_Ref';addpath(genpath(official));assert(hdrvdp_version()==3.07);pkg load image;pkg load statistics;
fftw('threads',3);fftw('planner','estimate');
original=fileread(fullfile(official,'hdrvdp3.m'));source=strrep(original,'function res = hdrvdp3(','function res = iqm_rgb_matrix_native(');assert(!strcmp(source,original));
assert(numel(strfind(source,'metric_par.base_dir = pathstr;'))==1);
source=strrep(source,'metric_par.base_dir = pathstr;',['metric_par.base_dir = ''' official ''';']);
assert(numel(strfind(source,'hdrvdp_visual_pathway('))==2);source=strrep(source,'hdrvdp_visual_pathway(','iqm_rgb_matrix_visual(');
visual=fileread(fullfile(official,'hdrvdp_visual_pathway.m'));visual=strrep(visual,' = hdrvdp_visual_pathway(',' = iqm_rgb_matrix_visual(');
visual=insert_once(visual,'M_img_lmsr = M_img_lmsr / sum(sum(M_img_lmsr(:,1:2)));',...
 'global iqm_rgb_matrix_trace;iqm_rgb_matrix_trace{end+1}=M_img_lmsr;');
f=fopen(fullfile(out,'iqm_rgb_matrix_native.m'),'w');assert(f>=0);fputs(f,source);fclose(f);
f=fopen(fullfile(out,'iqm_rgb_matrix_visual.m'),'w');assert(f>=0);fputs(f,visual);fclose(f);addpath(out,'-begin');
[lambda,s]=load_spectral_resp(fullfile(official,'data','log_cone_smith_pokorny_1975.csv'));s(s==0)=min(s(:));s=10.^s;
[~,s(:,4)]=load_spectral_resp(fullfile(official,'data','cie_scotopic_lum.txt'));
tl1=[.600,.510,.433,.377,.327,.295,.267,.233,.207,.187,.167,.147,.133,.120,.107,.093,.080,.067,.053,.040,.033,.027,.020,.013,.007,0];
tl2=[1,.583,.300,.116,.033,.005,zeros(1,20)];ages=[24,60,60.001];displays={'led-lcd-srgb','led-lcd-wcg','oled'};records={};
image=single(10)*ones(32,32,3,'single');global iqm_rgb_matrix_trace;
for display=displays
 name=display{1};[~,E]=load_spectral_resp(fullfile(official,'data',['emission_spectra_' name '.csv']));assert(isequal(size(E),[420,3]));
 f=fopen(fullfile(out,['samples_' name '.txt']),'w');assert(f>=0);fprintf(f,'%d\n',numel(lambda));fprintf(f,'%.17g\n',[lambda(:),E,s].');fclose(f);
 f=fopen(fullfile(out,['expected_' name '.txt']),'w');assert(f>=0);
 for age=ages
  if age<=60,od=tl1.*(1+.02*(age-32))+tl2;else,od=tl1.*(1.56+.0667*(age-60))+tl2;endif
  density=interp1(400:10:650,od-(tl1+tl2),clamp(lambda,400,650),'pchip');
  filtered=E.*repmat((10.^-density).',1,3);integrals=zeros(3,4);
  for channel=1:4,for color=1:3,integrals(color,channel)=trapz(lambda,s(:,channel).*filtered(:,color)*683.002);endfor;endfor
  matrix=integrals/sum(sum(integrals(:,1:2)));
  options={'use_gpu',false,'quiet',true,'age',age,'rgb_display',name};
  fftw('swisdom','');stocklog=evalc("stock=hdrvdp3('quality',image,image,'rgb-native',30,options);");
  iqm_rgb_matrix_trace={};fftw('swisdom','');tracelog=evalc("traced=iqm_rgb_matrix_native('quality',image,image,'rgb-native',30,options);");
  assert(isequal(stock.Q,traced.Q)&&isequal(stock.Q_JOD,traced.Q_JOD)&&stock.Q==10&&stock.Q_JOD==10);
  assert(numel(iqm_rgb_matrix_trace)==2);same(matrix,iqm_rgb_matrix_trace{1});same(matrix,iqm_rgb_matrix_trace{2});
  fprintf(f,'%.17g\n',age);fprintf(f,'%.17g\n',[reshape(integrals.',[],1);reshape(matrix.',[],1)]);
  records{end+1}=struct('display',name,'age',age,'samples',420,'native_matrices',2,'native_matrix_bit_errors',0,'Q',stock.Q,'Q_JOD',stock.Q_JOD,'min_emission',min(E(:)));
  fprintf('%s age%.17g:two official RGB matrices bitwise,identity Q10\n',name,age);
 endfor
 fclose(f);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);fputs(f,jsonencode(struct('version',3.07,'encoding','rgb-native','scope','Spectral calibration only;not RGB GPU or nonidentity quality acceptance','normalization','sum of L column then M column,across all3 primaries','official_source_sha256',hash('sha256',original),'records',{records})));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fprintf(f,'9RGB spectral configurations;18actual native3x4matrices bitwise;stock/traced identityscores equal.');fclose(f);
