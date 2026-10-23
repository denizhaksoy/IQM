% Exact native plans/fixtures for the first rectangular FFT contracts.
1;
function write_values(path,a)
 if !isreal(a)
  packed=zeros(rows(a),2*columns(a),'single');
  packed(:,1:2:end)=real(a);packed(:,2:2:end)=imag(a);
 else,packed=a;endif
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',size(packed));
 fprintf(f,'%.17g\n',packed');fclose(f);
endfunction
args=argv();out=args{1};probe=args{2};assert(!exist(out,'dir'));assert(mkdir(out));
addpath(probe);assert(exist('inspect_fftw_native','file')==3);
fftw('threads',3);fftw('swisdom','');rng(64256);
records={};
for shape={[128,256],[256,128],[8,16],[16,8]}
 dims=shape{1};h=dims(1);w=dims(2);tag=sprintf('h%d_w%d',h,w);
 folder=fullfile(out,tag);assert(mkdir(folder));
 a=single(randn(h,w));
 [forward,plan,counts,threads]=inspect_fftw_native(a,false);
 ordinary=fft2(a);assert(isequal(forward,ordinary));
 f=fopen(fullfile(folder,'forward_plan.txt'),'w');assert(f>=0);fputs(f,plan);fclose(f);
 inverse_input=complex(single(randn(h,w)),single(randn(h,w)));
 [inverse,plan,icounts,ithreads]=inspect_fftw_native(inverse_input,true);
 ordinary=ifft2(inverse_input);assert(isequal(inverse,ordinary));
 f=fopen(fullfile(folder,'inverse_plan.txt'),'w');assert(f>=0);fputs(f,plan);fclose(f);
 write_values(fullfile(folder,'real_input.txt'),a);
 write_values(fullfile(folder,'forward.txt'),forward);
 write_values(fullfile(folder,'inverse_input.txt'),inverse_input);
 write_values(fullfile(folder,'inverse.txt'),inverse);
 records{end+1}=struct('shape',dims,'forward_threads',threads,'inverse_threads',ithreads,...
  'forward_counts',counts,'inverse_counts',icounts,'exact_forward_components',2*numel(a),...
  'exact_inverse_components',2*numel(a),'inverse_normalization',1/numel(a));
 fprintf('%s exact native probe/fft2/ifft2;single threads%d/%d\n',tag,threads,ithreads);
endfor
f=fopen(fullfile(out,'metadata.json'),'w');assert(f>=0);
fputs(f,jsonencode(struct('scope','Native rectangular FFT fixtures only;no GPU pass',...
 'precision','single','padding','none;synthetic matrices already FFT-sized',...
 'layout','row-major text;complex interleaved columns','DC_processing','none',...
 'probe',which('inspect_fftw_native'),'planner',fftw('planner'),'records',{records})));fclose(f);
