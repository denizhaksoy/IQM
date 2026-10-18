1;
args=argv(); output=args{1}; assert(~exist(output,'dir')); assert(mkdir(output));
pkg load image;
fid=fopen(fullfile(output,'resize.txt'),'w'); assert(fid>=0);
df=fopen(fullfile(output,'resize_double.txt'),'w'); assert(df>=0);
count=0;
for shape={[64,64],[13,19],[1,17]}
  sz=shape{1}; [x,y]=meshgrid(0:sz(2)-1,0:sz(1)-1);
  for pattern=1:3
    if pattern==1
      input=single(1+0.2*sin(x*0.73)+0.1*cos(y*0.37)+x.*y/1000);
    elseif pattern==2
      input=zeros(sz,'single'); input(1,end)=17; input(end,1)=-3;
    else
      input=single((-1).^(x+y).*(1+x/17+y/23));
    endif
    targets=[sz;max(1,floor(sz/2));max(1,floor(sz/4));max(1,floor(sz/8));sz+[3,5]];
    for k=1:rows(targets)
      target=targets(k,:); result=imresize(input,target);
      assert(isa(result,'single'));
      fprintf(fid,'%d %d %d %d\n',sz(1),sz(2),target(1),target(2));
      % Explicit row-major serialization for the C++ test.
      fprintf(fid,'%.9g ',input'); fprintf(fid,'\n');
      fprintf(fid,'%.9g ',result'); fprintf(fid,'\n');
      double_result=imresize(double(input),target);
      fprintf(df,'%d %d %d %d\n',sz(1),sz(2),target(1),target(2));
      fprintf(df,'%.17g ',double(input')); fprintf(df,'\n');
      fprintf(df,'%.17g ',double_result'); fprintf(df,'\n');
      count+=1;
    endfor
  endfor
endfor
fclose(fid);
fclose(df);
fprintf('Octave %s / image imresize: %s\ncases=%d\n',version(),which('imresize'),count);
