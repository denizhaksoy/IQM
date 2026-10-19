% Independent stage oracle; leaves official files unchanged.
1;
args=argv();output=args{1};assert(!exist(output,'dir'));assert(mkdir(output));
reference='C:/HDR-VDP-3_Ref';if numel(args)>1,reference=args{2};endif
addpath(genpath(reference));assert(hdrvdp_version()==3.07);
[lambda,s]=load_spectral_resp(fullfile(reference,'data','log_cone_smith_pokorny_1975.csv'));
s(s==0)=min(s(:));s=10.^s;
[~,s(:,4)]=load_spectral_resp(fullfile(reference,'data','cie_scotopic_lum.txt'));
[~,emission]=load_spectral_resp(fullfile(reference,'data','d65.csv'));
if columns(emission)>1,emission=sum(emission,2);endif
f=fopen(fullfile(output,'samples.txt'),'w');assert(f>=0);fprintf(f,'%d\n',numel(lambda));
fprintf(f,'%.17g %.17g %.17g %.17g %.17g %.17g\n',[lambda(:),emission,s]');fclose(f);
tl1=[.600,.510,.433,.377,.327,.295,.267,.233,.207,.187,.167,.147,.133,.120,.107,.093,.080,.067,.053,.040,.033,.027,.020,.013,.007,0];
tl2=[1,.583,.300,.116,.033,.005,zeros(1,20)];
ages=[15,19.999,20,24,28,32,45,60,60.001,75,83,83.001,100];
f=fopen(fullfile(output,'expected.txt'),'w');assert(f>=0);
for age=ages
  if age<=60,od=tl1.*(1+.02*(age-32))+tl2;
  else,od=tl1.*(1.56+.0667*(age-60))+tl2;endif
  delta=od-(tl1+tl2);
  density=interp1(400:10:650,delta,clamp(lambda,400,650),'pchip');
  transmission=10.^-density;filtered=emission.*transmission';
  integrals=zeros(1,4);
  for ch=1:4,integrals(ch)=trapz(lambda,s(:,ch).*filtered*683.002);endfor
  matrix=integrals/sum(integrals(1:2));
  fprintf(f,'%.17g\n',age);
  fprintf(f,'%.17g\n',[delta(:);density(:);transmission(:);filtered(:);integrals(:);matrix(:)]);
endfor
fclose(f);fprintf('Spectral age oracle: %d ages, %d wavelengths\n',numel(ages),numel(lambda));
