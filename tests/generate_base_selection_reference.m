1;
% Independent Octave abs(complex single), DC suppression and first maximum.
args=argv(); output=args{1}; assert(!exist(output,'dir')); assert(mkdir(output));
rand('state',71804);
f=fopen(fullfile(output,'base_selection.txt'),'w'); assert(f>=0);
for k=0:1023
  r=single(2.^(floor(rand(8,8)*191)-90).*(1+rand(8,8)));
  im=single(2.^(floor(rand(8,8)*191)-90).*(1+rand(8,8)));
  if k<192
    % All component exponent differences, same-bin and adjacent mantissas.
    r=single((1+rand(8,8))*2^(mod(k,101)-50));
    im=single((1+rand(8,8))*2^(mod(k,101)-50-mod(k,16)));
  elseif k<256
    r(:)=0; im(:)=0;
    r(1,1)=single(2^100); % DC must not influence selection.
    if k>192
      r(2,1)=3; im(2,1)=4; r(1,2)=5;
      if mod(k,2), r(1,2)=typecast(typecast(single(5),'uint32')+uint32(1),'single'); endif
    endif
  elseif k<320
    % Equal rounded magnitudes from unequal exact squared norms.
    r(:)=1; im(:)=0; im(1,2)=single(2^-13);
  elseif k<384
    % Pythagorean and midpoint-adjacent values with random signs.
    r=single(randi([1,4095],8,8)); im=single(randi([1,4095],8,8));
  elseif k<448
    % Exact Pythagorean norm5*t is an odd integer above2^24: a true
    % binary32 midpoint. Alternating t exercises both ties-to-even choices.
    t=3355445+2*(k-384)+reshape(2*(0:63),8,8);
    r=single(3*t); im=single(4*t);
  endif
  r.*=single(2*(rand(8,8)>.5)-1); im.*=single(2*(rand(8,8)>.5)-1);
  spectrum=complex(r,im); magnitude=abs(spectrum); magnitude(1,1)=0;
  index=find(magnitude(:)==max(magnitude(:)),1);
  fprintf(f,'128\n'); packed=zeros(8,16,'single'); packed(:,1:2:end)=r; packed(:,2:2:end)=im;
  fprintf(f,'%.9g\n',packed'); fprintf(f,'%.9g\n',magnitude'); fprintf(f,'%d\n',index);
endfor
fclose(f);
fprintf('Native selection:1024 spectra,65536 rounded magnitudes and1024 first indices\n');
