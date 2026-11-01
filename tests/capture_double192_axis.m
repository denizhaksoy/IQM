% Native Double real192 batched first-axis characterization; fresh output only.
1;
args=argv();assert(numel(args)==2);out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));fftw('threads',3);fftw('planner','estimate');
fi=fopen(fullfile(out,'input.f64'),'wb','ieee-le');fe=fopen(fullfile(out,'expected.f64'),'wb','ieee-le');count=0;
for display={'led-lcd-srgb','led-lcd-wcg','oled'},for side=1:2,for convolution=1:4
 path=fullfile(source,[display{1} '_' num2str(side) '_conv' num2str(convolution) '_padded.f64']);f=fopen(path,'rb','ieee-le');assert(f>=0);a=fread(f,Inf,'double=>double');fclose(f);assert(numel(a)==192*192);a=reshape(a,[192,192]);
 fftw('dwisdom','');v=fft(a,[],1);assert(isa(v,'double'));v=v(1:97,:);fwrite(fi,a,'double');fwrite(fe,[real(v(:))';imag(v(:))'],'double');++count;
endfor;endfor;endfor
fclose(fi);fclose(fe);assert(count==24);f=fopen(fullfile(out,'fftw.wisdom'),'w');fputs(f,fftw('dwisdom'));fclose(f);
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native batched real192 first-axis only,not full2D or image-to-score','matrices',24,'batches',4608,'input_words',884736,'expected_words',893952,'precision','Double;FFTW3threads3/estimate/dwisdom cleared per matrix','layout','column-major real input,97complex first-axis outputs per column,interleaved real/imag','source',source)));fclose(f);
fprintf('Captured24Doublematrices;4608real192axis batches;893952expectedDoublewords\n');
