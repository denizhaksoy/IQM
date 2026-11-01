% Actual normalized real(ifft2) convolution stage boundary references.
1;args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));fftw('threads',3);fftw('planner','estimate');
fi=fopen(fullfile(out,'input.f64'),'wb','ieee-le');fe=fopen(fullfile(out,'expected_real.f64'),'wb','ieee-le');fa=fopen(fullfile(out,'expected_axis.f64'),'wb','ieee-le');count=0;
for display={'led-lcd-srgb','led-lcd-wcg','oled'},for side=1:2,for convolution=1:4
 prefix=fullfile(source,[display{1} '_' num2str(side) '_conv' num2str(convolution)]);f=fopen([prefix '_fft.f64'],'rb','ieee-le');assert(f>=0);z=fread(f,Inf,'double=>double');fclose(f);assert(numel(z)==73728);z=reshape(complex(z(1:2:end),z(2:2:end)),[192,192]);f=fopen([prefix '_filter.f64'],'rb','ieee-le');h=fread(f,Inf,'double=>double');fclose(f);assert(numel(h)==36864);input=z.*reshape(h,[192,192]);
 fftw('dwisdom','');axis=ifft(input,[],1).';fftw('dwisdom','');actual=real(ifft2(input));
 fwrite(fi,[real(input(:))';imag(input(:))'],'double');fwrite(fe,actual,'double');fwrite(fa,[real(axis(:))';imag(axis(:))'],'double');++count;
endfor;endfor;endfor;fclose(fi);fclose(fe);fclose(fa);assert(count==24);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native normalized real(ifft2) filtering boundary;axis oracle is normalized1D not reconstructed raw','precision','Double/FFTW3threads3/estimate/dwisdom cleared','matrices',24,'source',source,'layout','column-major complex interleaved;first-axis output transposed per192matrix;final real column-major','parameters','original Single96sq/30ppd/age24/defaultsp0;symmetric optical/replicate adaptation postpadding;DC unchanged;final inverse divisor36864')));fclose(f);fprintf('Captured24normalized Double inverse matrices\n');
