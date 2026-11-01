% Isolated Double complex second-axis / complete forward reference packet.
1;args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));fftw('threads',3);fftw('planner','estimate');
fi=fopen(fullfile(out,'input.f64'),'wb','ieee-le');fe=fopen(fullfile(out,'expected.f64'),'wb','ieee-le');fp=fopen(fullfile(out,'padded.f64'),'wb','ieee-le');count=0;
for display={'led-lcd-srgb','led-lcd-wcg','oled'},for side=1:2,for convolution=1:4
 f=fopen(fullfile(source,[display{1} '_' num2str(side) '_conv' num2str(convolution) '_padded.f64']),'rb','ieee-le');assert(f>=0);a=fread(f,Inf,'double=>double');fclose(f);assert(numel(a)==36864);a=reshape(a,[192,192]);fftw('dwisdom','');partial=fft(a,[],1);fftw('dwisdom','');full=fft2(a);partial=partial(1:97,:);
 fwrite(fi,[real(partial(:))';imag(partial(:))'],'double');fwrite(fe,[real(full(:))';imag(full(:))'],'double');fwrite(fp,a,'double');++count;
endfor;endfor;endfor;fclose(fi);fclose(fe);fclose(fp);assert(count==24);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Double192x192 forward;partial input isolates second-axis;not image-to-score','matrices',24,'precision','Double;FFTW3threads3/estimate/dwisdom cleared','layout','column-major;partial97complex percolumn,full192complex percolumn,interleaved real/imag','parameters','Single encoded96sq/30ppd/age24/defaultsp0 source;symmetric optical and replicate adaptation postpadding;forward unnormalized;DC unchanged','source',source)));fclose(f);fprintf('Captured24Double2Dforward matrices\n');
