% Original-size Double cases for development of generic C++ orchestration.
% These are engineering cases, not substitutes for missing UPIQ data.
1;
function write_rows(path,value)
  f=fopen(path,'wb','ieee-le');assert(f>=0);fwrite(f,permute(double(value),[2,1,3]),'double');fclose(f);
end
args=argv();assert(numel(args)==3,'Usage: clean-reference fresh-output quick|extended');
official=canonicalize_file_name(args{1});out=args{2};assert(~exist(out,'dir'));assert(mkdir(out));
addpath(genpath(official));pkg load image;pkg load statistics;assert(hdrvdp_version()==3.07);
fftw('threads',3);fftw('planner','estimate');
source=fileread(fullfile(official,'hdrvdp3.m'));
source=strrep(source,'function res = hdrvdp3(','function res = iqm_general_metric(');
anchor='metric_par.base_dir = pathstr;';assert(numel(strfind(source,anchor))==1);source=strrep(source,anchor,['metric_par.base_dir = ''' strrep(official,'\','/') ''';']);
anchor='log_La = log10(clamp( L_mean_adapt, CSF.csf_la(1), CSF.csf_la(end) ));';assert(numel(strfind(source,anchor))==1);
capture=['global iqm_general_trace;' char(10) 'iqm_general_trace=struct(''reference_rgb'',reference,''test_rgb'',test,''reference_P'',P_ref,''test_P'',P_test,''reference_adaptation'',L_adapt_reference,''test_adaptation'',L_adapt_test,''mean_log'',log_La,''r_pyramid'',B_R,''t_pyramid'',B_T);'];
source=strrep(source,anchor,[anchor char(10) capture]);
anchor='Q_err = Q_err + minkowski_sum( D_p(:), 0.8 )/B_T.band_count();';assert(numel(strfind(source,anchor))==1);
source=strrep(source,anchor,[anchor char(10) 'iqm_general_trace.errors(b)=minkowski_sum(D_p(:),.8);']);
f=fopen(fullfile(out,'iqm_general_metric.m'),'wt');fputs(f,source);fclose(f);addpath(out);
if strcmp(args{3},'quick')
  configs={80,64,30,24,'rgb-native','oled';97,65,30,24,'rgb-native','oled';96,96,30,24,'sRGB-display','led-lcd-srgb'};
elseif strcmp(args{3},'extended')
  configs={256,192,60,45,'rgb-native','oled';1024,683,30,24,'sRGB-display','led-lcd-srgb'};
elseif strcmp(args{3},'linear')
  configs={96,80,45,35,'rgb-bt.709','oled';96,80,45,35,'rgb-bt.2020','oled';65,97,30,60,'XYZ','oled'};
else,error('Unknown scope');end
records={};
for k=1:size(configs,1)
  w=configs{k,1};h=configs{k,2};ppd=configs{k,3};age=configs{k,4};encoding=configs{k,5};display=configs{k,6};
  [x,y]=meshgrid((0:w-1)/(w-1),(0:h-1)/(h-1));
  if w==1024
    raw=imread(fullfile(official,'examples','wavy_facade.png'));reference=double(raw)/double(intmax(class(raw)));
  elseif strcmp(encoding,'sRGB-display')
    reference=cat(3,.01+.7*x,.05+.8*y,.1+.3*x+.4*y);
  else
    reference=cat(3,.05+9*x.^2+2*y,.1+3*y.^2+x,.2+x+2*y);
  end
  test=reference;yr=floor(h*.3):ceil(h*.7);xr=floor(w*.35):ceil(w*.75);test(yr,xr,2)=test(yr,xr,2)*.8;
  name=sprintf('case%d_%dx%d_%gppd_age%g',k,w,h,ppd,age);dir=fullfile(out,name);assert(mkdir(dir));
  f=fopen(fullfile(dir,'config.txt'),'wt');fprintf(f,'%d %d %.17g %.17g %s %s\n',w,h,ppd,age,encoding,display);fclose(f);
  write_rows(fullfile(dir,'reference.f64'),reference);write_rows(fullfile(dir,'test.f64'),test);
  options={'use_gpu',false,'quiet',true,'age',age,'rgb_display',display};
  fftw('swisdom','');fftw('dwisdom','');stockLog=evalc("stock=hdrvdp3('quality',test,reference,encoding,ppd,options);");
  global iqm_general_trace;fftw('swisdom','');fftw('dwisdom','');traceLog=evalc("captured=iqm_general_metric('quality',test,reference,encoding,ppd,options);");
  assert(strcmp(num2hex(stock.Q),num2hex(captured.Q))&&strcmp(num2hex(stock.Q_JOD),num2hex(captured.Q_JOD)),'Instrumented copy changed clean stock scores');
  for field={'reference_rgb','test_rgb','reference_P','test_P','reference_adaptation','test_adaptation','mean_log'}
    write_rows(fullfile(dir,[field{1},'.f64']),iqm_general_trace.(field{1}));
  end
  for side={'reference','test'}
    if strcmp(side{1},'reference'),pyr=iqm_general_trace.r_pyramid;else,pyr=iqm_general_trace.t_pyramid;end
    for b=1:pyr.band_count(),write_rows(fullfile(dir,sprintf('%s_band%d.f64',side{1},b)),pyr.get_band(b,1));end
  end
  for b=1:numel(iqm_general_trace.errors),write_rows(fullfile(dir,sprintf('band%d_error.f64',b)),iqm_general_trace.errors(b));end
  write_rows(fullfile(dir,'scores.f64'),[stock.Q stock.Q_JOD]);
  f=fopen(fullfile(dir,'stock.log'),'wt');fputs(f,stockLog);fclose(f);f=fopen(fullfile(dir,'trace.log'),'wt');fputs(f,traceLog);fclose(f);
  records{end+1}=struct('id',name,'width',w,'height',h,'ppd',ppd,'age',age,'encoding',encoding,'display',display,'Q_text',sprintf('%.17g',stock.Q),'Q_JOD_text',sprintf('%.17g',stock.Q_JOD),'scope','engineering-not-UPIQ');
  fprintf('%s clean-stock Q=%.17g Q_JOD=%.17g\n',name,stock.Q,stock.Q_JOD);
end
f=fopen(fullfile(out,'records.json'),'wt');fputs(f,jsonencode(records));fclose(f);
