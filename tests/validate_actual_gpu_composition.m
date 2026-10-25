1;
function a=read_single(path,words)
 f=fopen(path,'r');assert(f>=0);shape=fscanf(f,'%d',2);v=fscanf(f,'%f');fclose(f);assert(numel(v)==prod(shape));if words,a=uint32(v);else,a=typecast(single(v),'uint32');endif
endfunction
args=argv();assert(numel(args)==6);out=args{1};filters=args{2};comparison=args{3};quality=args{4};trace=args{5};visual=args{6};assert(!exist(out,'dir'));assert(mkdir(out));records={};table_hashes=struct();
for hw={[80,96],[96,80]}
 shape=hw{1};tag=sprintf('h%d_w%d',shape(1),shape(2));loaded=load(fullfile(trace,tag,'trace.mat'));
 for label={'reference','test'}
  n=label{1};short='ref';if strcmp(n,'test'),short='test';endif
  input=read_single(fullfile(filters,tag,[short '_luminance.txt']),false);
  binary_input=read_single(fullfile(comparison,[tag '_p0'],[n '.txt']),true);assert(isequal(input,binary_input));
  for field={'matrix','pn_Y1','pn_Y2','pn_JND1','pn_JND2'}
   k=field{1};digest=hash('sha256',fileread(fullfile(visual,'stages',tag,[n '_' k '.txt'])));
   if isfield(table_hashes,k),assert(strcmp(table_hashes.(k),digest));else,table_hashes.(k)=digest;endif
  endfor
 endfor
 f=fopen(fullfile(quality,[tag '_quality_scores.f32']),'rb','ieee-le');assert(f>=0);bits=fread(f,Inf,'*uint32');fclose(f);
 expected=typecast(single([loaded.baseline.Q;loaded.baseline.Q_JOD]),'uint32');assert(isequal(bits,expected));
 scores=typecast(bits,'single');records{end+1}=struct('shape',shape,'score_bits',bits.','Q',double(scores(1)),'Q_JOD',double(scores(2)),'score_bit_identity',true);
endfor
f=fopen(fullfile(out,'results.json'),'w');fputs(f,jsonencode(struct('scope','File-composed experimental image-to-score validation; native image-dependent intermediates are goldens only',...
 'records',{records},'fixed_matrix_and_JND_tables_identical_across_all_images',true,'fixed_model_table_hashes',table_hashes,...
 'original_filter_and_comparison_inputs_bit_identical',true,'CPU_work','model tables/axis plans, double NCSF, serialization and validation',...
 'remaining','Public API integration, broader geometry/content/age/ppd coverage, removal of file/host stage transfers')));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Two image pairs: original input provenance and final Q/Q_JOD bit identity PASS; fixed model tables invariant across images.');fclose(f);
