1;
function [v,d]=read_real_values(path)
  f=fopen(path,'r');assert(f>=0,path);d=fscanf(f,'%d',2);v=single(fscanf(f,'%f'));fclose(f);assert(numel(v)==prod(d));
endfunction
function v=read_column_complex(path,row_order)
  [v,d]=read_real_values(path);n=d(1);assert(d(2)==2*n);
  if row_order
    z=complex(v(1:2:end),v(2:2:end));z=reshape(z,n,n).';z=z(:);
    a=[real(z)';imag(z)'];v=a(:);
  endif
endfunction
args=argv(); native=args{1}; gpu=args{2}; total=0; count=0;
adaptation=numel(args)>2&&strcmp(args{3},'--adaptation');
mode='mtf';if adaptation,mode='la';endif
for label={'ref','test'}
  for stage=0:3
    if stage==0||stage==2
      expected=read_column_complex(fullfile(native,sprintf('%s_%s_stage%d.txt',label{1},mode,stage)),false);
    else
      direction='forward';if stage==3,direction='inverse';endif
      prefix=[label{1} '_native_'];if adaptation,prefix=[label{1} '_la_native_'];endif
      expected=read_column_complex(fullfile(native,[prefix direction '.txt']),true);
    endif
    actual=read_column_complex(fullfile(gpu,sprintf('%s_%s_stage%d.txt',label{1},mode,stage)),false);
    assert(numel(actual)==numel(expected));delta=double(actual)-double(expected);n=nnz(delta);total+=n;count+=numel(delta);
    fprintf('%s stage%d values=%d mismatches=%d max=%.17g\n',label{1},stage,numel(delta),n,max(abs(delta)));
  endfor
  kinds={'retinal','mtf_filter'};if adaptation,kinds={'adapt_input','adaptation','la_filter'};endif
  for kind=kinds
    if isempty(strfind(kind{1},'filter'))
      expected=read_real_values(fullfile(native,[label{1} '_' kind{1} '.txt']));actual=read_real_values(fullfile(gpu,[label{1} '_' kind{1} '.txt']));
    else
      expected=read_real_values(fullfile(native,[mode '_filter_double.txt']));actual=read_real_values(fullfile(gpu,[label{1} '_' mode '_filter.txt']));
    endif
    delta=double(actual)-double(expected);n=nnz(delta);total+=n;count+=numel(delta);
    fprintf('%s %s values=%d mismatches=%d max=%.17g\n',label{1},kind{1},numel(delta),n,max(abs(delta)));
  endfor
endfor
result=struct('scope',[mode ' preprocessing only;not full metric'],'float_values',count,'float_mismatches',total,'native',native);
f=fopen(fullfile(gpu,[mode '_comparison.json']),'w');fputs(f,jsonencode(result));fclose(f);
fprintf('%s values=%d mismatches=%d\n',mode,count,total);assert(total==0,'FFT preprocessing pipeline parity failed');
