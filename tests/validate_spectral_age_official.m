% Cross-check stage-oracle matrices with the actual official quality call.
% Official diagnostic prints have 15 decimal places, not full double precision.
1;
args=argv();root=args{1};reference='C:/HDR-VDP-3_Ref';
if numel(args)>1,reference=args{2};endif
addpath(genpath(reference));pkg load image;pkg load statistics;
assert(hdrvdp_version()==3.07);
f=fopen(fullfile(root,'samples.txt'));assert(f>=0);n=fscanf(f,'%d',1);fclose(f);
f=fopen(fullfile(root,'expected.txt'));assert(f>=0);count=0;
image=repmat(single(10),32);
while true
  age=fscanf(f,'%f',1);if isempty(age),break;endif
  trace=fscanf(f,'%f',26+3*n+8);assert(numel(trace)==26+3*n+8);
  matrix=trace(end-3:end)';
  log=evalc("result=hdrvdp3('quality',image,image,'luminance',30,{'age',age,'use_gpu',false,'quiet',true});");
  lines=regexp(log,'=== LMSR MATRIX ===\s*([^\r\n]+)','tokens');assert(numel(lines)==2);
  expected=strtrim(sprintf('%.15f %.15f %.15f %.15f',matrix));
  for k=1:2
    assert(strcmp(strtrim(lines{k}{1}),expected),'Official matrix diagnostic differs');
    actual=sscanf(lines{k}{1},'%f')';assert(isequal(single(actual),single(matrix)));
  endfor
  assert(result.Q==10&&result.Q_JOD==10);
  fprintf('age=%.17g official_matrix_prints=2 single_coefficients=8 exact identical_Q=10\n',age);
  count+=1;
endwhile
fclose(f);assert(count==13);fprintf('Official integration: %d ages, %d matrices\n',count,2*count);
