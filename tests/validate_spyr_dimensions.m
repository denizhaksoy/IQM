% Isolated raw single SPYR dimension validation; not full metric validation.
1;
function image=read_row_map(path)
  f=fopen(path,'r'); assert(f>=0,path); dims=fscanf(f,'%d',2); values=fscanf(f,'%f'); fclose(f);
  assert(numel(values)==prod(dims));
  image=single(reshape(values,dims(2),dims(1))');
endfunction
args=argv(); root=args{1}; official='C:/HDR-VDP-3_Ref';
if numel(args)>1, official=args{2}; endif
addpath(genpath(official)); assert(hdrvdp_version()==3.07);
f=fopen(fullfile(root,'cases.txt')); assert(f>=0); names=textscan(f,'%s'); fclose(f);
records={}; total=0; centered_total=0; pixels=0;
for k=1:numel(names{1})
  name=names{1}{k}; directory=fullfile(root,name);
  input=read_row_map(fullfile(directory,'input.txt'));
  f=fopen(fullfile(directory,'parameters.txt')); parameters=fscanf(f,'%f'); fclose(f);
  ppd=parameters(1); height=min(ceil(log2(ppd))-2,maxPyrHt(size(input),13));
  assert(parameters(2)==height && parameters(3)==height+2);
  [pyr,pind]=buildSpyr(input,height,'sp0Filters','reflect1');
  case_mismatches=0; case_centered=0;
  for b=0:rows(pind)-1
    native=pyrBand(pyr,pind,b+1);
    gpu=read_row_map(fullfile(directory,sprintf('gpu_band%d.txt',b)));
    assert(isequal(size(native),size(gpu)) && isa(native,'single'));
    assert(all(isfinite(native(:))) && all(isfinite(gpu(:))));
    delta=double(gpu)-double(native); n=nnz(delta);
    case_mismatches+=n; pixels+=numel(native);
    fprintf('%s raw band%d shape=%dx%d values=%d mismatches=%d max=%.17g\n',name,b,size(native),numel(native),n,max(abs(delta(:))));
    if b==rows(pind)-1
      % Match existing host DC processing and the official single mean.
      centered_gpu=gpu-single(mean(double(gpu(:))));
      centered_native=native-mean(native(:));
      case_centered=nnz(centered_gpu!=centered_native);
      fprintf('%s residual DC meanGPU=%.17g meanOctave=%.17g centered_mismatches=%d\n',name,single(mean(double(gpu(:)))),mean(native(:)),case_centered);
    endif
  endfor
  records{end+1}=struct('name',name,'ppd',ppd,'height',height,'band_shapes',pind,'raw_mismatches',case_mismatches,'centered_mismatches',case_centered);
  total+=case_mismatches; centered_total+=case_centered;
endfor
result=struct('scope','Raw single SPYR only; no MTF/JND/masking/quality','reference_version',hdrvdp_version(),'padding','reflect1','layout','C++ row-major text reshaped to Octave column-major','dc','Raw bands compared first; final residual independently centered','cases',{records},'raw_values',pixels,'raw_mismatches',total,'centered_mismatches',centered_total);
f=fopen(fullfile(root,'dimension_summary.json'),'w'); assert(f>=0); fputs(f,jsonencode(result)); fclose(f);
fprintf('TOTAL raw_values=%d raw_mismatches=%d centered_mismatches=%d\n',pixels,total,centered_total);
assert(total==0 && centered_total==0,'SPYR dimension parity failed; see per-band diagnostics');
