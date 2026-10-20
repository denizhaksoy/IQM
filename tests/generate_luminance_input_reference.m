1;
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));
list=fopen(fullfile(out,'cases.txt'),'w');assert(list>=0);k=0;
for shape={[3,5],[32,32],[64,64]}
  h=shape{1}(1);w=shape{1}(2);image=single(reshape(1:h*w,w,h)'/7);
  for little=[false,true]
    for magnitude=[1,2,.125]
      k+=1;name=sprintf('case%d',k);scale=magnitude;if little,scale=-scale;endif
      machine='ieee-be';if little,machine='ieee-le';endif
      f=fopen(fullfile(out,[name '.pfm']),'wb',machine);fprintf(f,'Pf\n%d %d\n%.9g\n',w,h,scale);fwrite(f,flipud(image)','single');fclose(f);
      f=fopen(fullfile(out,[name '.txt']),'w');fprintf(f,'%d %d\n',h,w);fprintf(f,'%.9g\n',(image*single(magnitude))');fclose(f);
      fprintf(list,'%s.pfm %s.txt\n',name,name);
    endfor
  endfor
endfor
bad={'color','zero_scale','zero_dimension','truncated','zero_pixel','nan_pixel','trailing'};
for i=1:numel(bad)
  name=[bad{i} '.pfm'];f=fopen(fullfile(out,name),'wb','ieee-le');
  magic='Pf';if i==1,magic='PF';endif
  scale=-1;if i==2,scale=0;endif
  width=1;if i==3,width=0;endif
  fprintf(f,'%s\n%d 1\n%d\n',magic,width,scale);
  if i!=4,value=single(1);if i==5,value=single(0);elseif i==6,value=single(NaN);endif;fwrite(f,value,'single');endif
  if i==7,fwrite(f,uint8(1),'uint8');endif
  fclose(f);fprintf(list,'%s reject\n',name);
endfor
fclose(list);fprintf('PFM cases:%d valid and%d rejected\n',k,numel(bad));
