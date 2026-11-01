% Native-only seed acceptance for the next geometry; no GPU parity claim.
1;
function equal_bits(a,b)
 assert(isequal(size(a),size(b))&&strcmp(class(a),class(b)));
 if isa(a,'single'),word='uint32';else,assert(isa(a,'double'));word='uint64';endif
 assert(isequal(typecast(real(a(:)),word),typecast(real(b(:)),word)));
 assert(isequal(typecast(imag(a(:)),word),typecast(imag(b(:)),word)));
endfunction
function complex_text(file,a)
 f=fopen(file,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),2*columns(a));
 % Extract whole arrays first: indexing a complex scalar can narrow a zero
 % imaginary component to real and discard its negative-zero sign in Octave.
 ar=real(a);ai=imag(a);
 for y=1:rows(a),for x=1:columns(a),fprintf(f,'%.17g\n%.17g\n',ar(y,x),ai(y,x));endfor;endfor
 fclose(f);
endfunction
args=argv();assert(numel(args)==2);root=args{1};out=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);
fftw('threads',3);fftw('planner','estimate');records={};
for label={'asymmetric','impulse','hdr_ramp','constant','identity'}
 name=label{1};folder=fullfile(root,name,'h96_w96');m=jsondecode(fileread(fullfile(root,name,'metadata.json')));
 assert(numel(m.records)==1&&isequal(m.records.shape(:),[96;96])&&m.records.score_identity);
 a=load(fullfile(folder,'trace.mat'));t=a.trace;b=t.base;
 assert(rows(t.bands)==5&&isequal(size(b.difference),[12,12])&&isa(b.difference,'single'));
 fftw('swisdom','');raw=fft2(b.difference);spectrum=raw;spectrum(1,1)=0;
 equal_bits(spectrum,b.spectrum);
 rho=create_cycdeg_image(size(b.difference),2^-(rows(t.bands)-1)*30/2*4);
 magnitudes=abs(spectrum(:));selected=find(magnitudes==max(magnitudes),1);
 assert(selected==b.index);equal_bits(rho(selected),b.frequency);
 equal_bits(10.^mean(t.log_resized{end}(:)),b.L_mean);
 if strcmp(name,'identity')
  assert(a.baseline.Q==10&&a.baseline.Q_JOD==10&&all(b.difference(:)==0));
  assert(selected==1&&b.frequency==0&&isinf(b.N_nCSF)&&b.N_nCSF>0);
 else,assert(isfinite(b.N_nCSF)&&b.N_nCSF>0);endif
 target=fullfile(out,name);assert(mkdir(target));
 complex_text(fullfile(target,'input.txt'),b.difference);
 complex_text(fullfile(target,'expected.txt'),raw);
 complex_text(fullfile(target,'expected_dc_zero.txt'),spectrum);
 f=fopen(fullfile(target,'swisdom.txt'),'w');assert(f>=0);fputs(f,fftw('swisdom'));fclose(f);
 records{end+1}=struct('case',name,'shape',[96,96],'base_shape',[12,12],...
  'fft_real_imag_words',288,'bit_errors',0,'column_major_index',selected,...
  'maximum_ties',sum(magnitudes==max(magnitudes)),'frequency',b.frequency,...
  'positive_infinite_noise',isinf(b.N_nCSF),'Q',a.baseline.Q,'Q_JOD',a.baseline.Q_JOD);
endfor
f=fopen(fullfile(out,'results.json'),'w');assert(f>=0);fputs(f,jsonencode(records));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');assert(f>=0);fputs(f,'Five native96x96 seeds: stock/traced scores equal,1440 replayed base FFT words bitwise exact,DC/frequency/index/adaptation checked;identity zero/+Inf limit checked. No GPU acceptance.');fclose(f);
