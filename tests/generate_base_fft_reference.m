1;
% Native8x8 FFT2 fixture for the next base-frequency port. Not a GPU pass.
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
addpath(genpath('C:/HDR-VDP-3_Ref')); rand('state',71502);
if numel(args)>1, addpath(args{2}); endif
[xx,yy]=meshgrid(0:7,0:7); rho=create_cycdeg_image([8,8],3.75);
f=fopen(fullfile(output,'base_fft.txt'),'w'); assert(f>=0);
packed_in=fopen(fullfile(output,'input128.txt'),'w'); packed_out=fopen(fullfile(output,'forward128.txt'),'w');
assert(packed_in>=0 && packed_out>=0); fprintf(packed_in,'64 256\n'); fprintf(packed_out,'64 256\n');
for k=0:63
  x=single((rand(8,8)-.5)*2^(-10+mod(k,21)));
  if k==0, x(:)=0;
  elseif k==1, x(:)=1;
  elseif k==2, x(:)=0; x(3,6)=1;
  elseif k==3, x=single(cos(2*pi*xx/8));
  elseif k==4, x=single(cos(2*pi*yy/8));
  elseif k==5, x=single(cos(2*pi*xx/8)+cos(4*pi*yy/8));
  elseif k==6, x=single((-1).^(xx+yy));
  elseif k==7, x=single(cos(2*pi*xx/8)+cos(4*pi*yy/8)); x(3,6)+=eps('single'); endif
  spectrum=fft2(x); selection=spectrum; selection(1,1)=0;
  index=find(abs(selection(:))==max(abs(selection(:))),1);
  fprintf(f,'8 8 %d %.17g\n',index,rho(index));
  fprintf(f,'%.9g\n',x');
  interleaved=zeros(8,16,'single'); interleaved(:,1:2:end)=real(spectrum); interleaved(:,2:2:end)=imag(spectrum);
  fprintf(f,'%.9g\n',interleaved');
  packed=zeros(128,2,'single'); packed(1:64,1)=reshape(x.',[],1);
  fprintf(packed_in,'%.9g\n',packed');
  packed(:)=0; row_spectrum=reshape(spectrum.',[],1);
  packed(1:64,:)=[real(row_spectrum),imag(row_spectrum)]; fprintf(packed_out,'%.9g\n',packed');
  if k==0 && numel(args)>1
    [actual,plan,counts,threads]=inspect_fftw_native(x,false);
    assert(nnz(actual!=spectrum)==0);
    fid=fopen(fullfile(output,'native_plan.txt'),'w'); assert(fid>=0);
    fprintf(fid,'%s\nthreads=%d adds=%g muls=%g fmas=%g\n',plan,threads,counts); fclose(fid);
  endif
endfor
fclose(f);
fclose(packed_in); fclose(packed_out);
fprintf('Native base FFT fixture:64 real8x8 inputs, spectra and first-max selections; effective ppd3.75\n');
