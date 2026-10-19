1;
function values=read_values(path,column_order)
  f=fopen(path,'r'); assert(f>=0); d=fscanf(f,'%d',2); v=fscanf(f,'%f'); fclose(f);
  n=d(1); assert(d(2)==2*n && numel(v)==2*n*n);
  values=complex(single(v(1:2:end)),single(v(2:2:end)));
  if !column_order,values=reshape(values,n,n).';values=values(:);endif
endfunction
function write_values(path,values)
  f=fopen(path,'w'); assert(f>=0); n=sqrt(numel(values));fprintf(f,'%d %d\n',n,2*n);
  components=[real(values)';imag(values)'];fprintf(f,'%.17g\n',components);fclose(f);
endfunction
args=argv(); source=args{1}; output=args{2};assert(!exist(output,'dir'));assert(mkdir(output));
for label={'ref','test'}
  for direction={'forward','inverse'}
    directory=fullfile(output,[label{1} '_' direction{1}]);assert(mkdir(directory));
    stage='0';if strcmp(direction{1},'inverse'),stage='2';endif
    input=read_values(fullfile(source,[label{1} '_mtf_stage' stage '.txt']),true);
    expected=read_values(fullfile(source,[label{1} '_native_' direction{1} '.txt']),false);
    write_values(fullfile(directory,'input64.txt'),input);
    write_values(fullfile(directory,[direction{1} '64.txt']),expected);
  endfor
endfor
