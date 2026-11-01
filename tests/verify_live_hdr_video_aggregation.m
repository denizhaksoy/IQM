% Independently check the wrapper's ordered Single Q_JOD accumulation in Octave.
args=argv();assert(numel(args)==1);root=args{1};
target=fullfile(root,'aggregation_octave.json');assert(~exist(target,'file'));
video=jsondecode(fileread(fullfile(root,'video_score.json')));
assert(video.complete&&video.exact&&video.frames==numel(video.indices));
reference_sum=0;cpp_sum=0;n=0;
for index=video.indices(:)'
 entry=jsondecode(fileread(fullfile(root,sprintf('frame%04d',index),'comparison.json')));
 assert(entry.exact&&entry.zero_based_frame==index);
 reference=single(str2double(entry.official.Q_JOD_text));
 actual=single(str2double(entry.cpp.Q_JOD_text));
 assert(strcmp(num2hex(reference),entry.official.Q_JOD_hex));
 assert(strcmp(num2hex(actual),entry.cpp.Q_JOD_hex));
 reference_sum=reference_sum+reference;cpp_sum=cpp_sum+actual;n=n+1;
end
reference_mean=reference_sum/n;cpp_mean=cpp_sum/n;
assert(isa(reference_mean,'single')&&isa(cpp_mean,'single'));
assert(strcmp(num2hex(reference_mean),num2hex(cpp_mean)));
assert(strcmp(num2hex(reference_mean),num2hex(single(video.official_Q_JOD))));
assert(strcmp(num2hex(cpp_mean),num2hex(single(video.cpp_Q_JOD))));
result=struct('frames',n,'class',class(reference_mean),'Q_JOD_text',sprintf('%.17g',reference_mean),...
 'Q_JOD_hex',num2hex(reference_mean),'exact',true,'octave_version',version());
f=fopen(target,'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);disp(jsonencode(result));
