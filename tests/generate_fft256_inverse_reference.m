% Native single complex inverse256 fixtures only; no GPU/full metric claim.
1;
function write_map(path,image,column_order)
  f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(image),2*columns(image));
  if !column_order,image=image.';endif
  v=image(:);fprintf(f,'%.17g\n',[real(v)';imag(v)']);fclose(f);
endfunction
args=argv();output=args{1};assert(!exist(output,'dir'));assert(mkdir(output));
rng(2562026);fftw('swisdom','');
modes={'zero','impulse','random_small','random_unit','random_large','random_subnormal','random_boundary'};
records={};
for mode=modes
  name=mode{1};directory=fullfile(output,name);assert(mkdir(directory));
  for label={'ref','test'}
    tag=label{1};
    if strcmp(name,'zero'),input=complex(zeros(256,256,'single'));
    elseif strcmp(name,'impulse')
      input=complex(zeros(256,256,'single'));input(7,29)=complex(single(3.125),single(-1.75));
      if strcmp(tag,'test'),input(241,4)=complex(single(-0.75),single(2.5));endif
    else
      scale=1;if strcmp(name,'random_small'),scale=1e-3;elseif strcmp(name,'random_large'),scale=1e3;endif
      if strcmp(name,'random_subnormal'),scale=1e-40;elseif strcmp(name,'random_boundary'),scale=1e-38;endif
      input=complex(single(scale*randn(256,256)),single(scale*randn(256,256)));
    endif
    expected=ifft2(input);assert(isa(expected,'single'));
    % Compatibility filenames consumed by the existing inverse-stage harness;
    % these are arbitrary complex inverse inputs, not MTF-filtered images.
    write_map(fullfile(directory,[tag '_mtf_stage2.txt']),input,true);
    write_map(fullfile(directory,[tag '_native_inverse.txt']),expected,false);
    records{end+1}=struct('case',name,'label',tag,'values',2*numel(input));
  endfor
endfor
f=fopen(fullfile(output,'native_single_wisdom.txt'),'w');fputs(f,fftw('swisdom'));fclose(f);
f=fopen(fullfile(output,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native inverse256 only;not GPU/full metric','precision','single','input_layout','column-major interleaved','expected_layout','row-major interleaved','normalization',1/65536,'threads',fftw('threads'),'planner',fftw('planner'),'records',{records})));fclose(f);
fprintf('Native inverse256:14 matrices,1835008 float components\n');
