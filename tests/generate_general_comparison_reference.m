1;
function write_words(path,a)
 f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),columns(a));fprintf(f,'%u\n',typecast(reshape(a.',[],1),'uint32'));fclose(f);
endfunction
args=argv();out=args{1};source=args{2};assert(!exist(out,'dir'));assert(mkdir(out));pkg load image;rng(248096);
manifest=fopen(fullfile(out,'manifest.txt'),'w');assert(manifest>=0);cases=0;values=0;
for hw={[80,96],[96,80]}
 shape=hw{1};h=shape(1);w=shape(2);tag=sprintf('h%d_w%d',h,w);loaded=load(fullfile(source,tag,'trace.mat'));t=loaded.trace;
 for pattern=0:8
  reference=single(10)*ones(h,w,'single');test=reference;
  if pattern==0
   f=fopen(fullfile(source,tag,'reference.txt'));fscanf(f,'%d',2);reference=single(reshape(fscanf(f,'%f'),w,h).');fclose(f);
   f=fopen(fullfile(source,tag,'test.txt'));fscanf(f,'%d',2);test=single(reshape(fscanf(f,'%f'),w,h).');fclose(f);
  else
   mask=false(h,w);[x,y]=meshgrid(0:w-1,0:h-1);
   if pattern==2,mask(:)=true;elseif pattern==3,mask(1,1)=true;elseif pattern==4,mask(end,end)=true;
   elseif pattern==5,mask=mod(x+y,2)==0;elseif pattern==6,mask=rand(h,w)>.6;elseif pattern==7,mask=mod(x,7)==0;endif
   test(mask)=single(20);
   if pattern==8,test(:)=single(10.01);bits=typecast(test(:),'uint32');bits(1:3:end)-=uint32(1);bits(2:3:end)+=uint32(1);test=reshape(typecast(bits,'single'),h,w);endif
  endif
  binary=double(abs(test-reference)./reference>.001);name=sprintf('%s_p%d',tag,pattern);folder=fullfile(out,name);assert(mkdir(folder));
  write_words(fullfile(folder,'reference.txt'),reference);write_words(fullfile(folder,'test.txt'),test);
  f=fopen(fullfile(folder,'binary.txt'),'w');fprintf(f,'%d %d\n',h,w);fprintf(f,'%u\n',uint32(binary.'));fclose(f);
  for b=1:5
   target=size(t.masking{b,1}.comparison);expected=imresize(binary,target);
   if pattern==0,assert(isequal(typecast(expected(:),'uint64'),typecast(t.masking{b,1}.comparison(:),'uint64')));endif
   path=strrep(fullfile(folder,sprintf('band%d.txt',b)),'\','/');f=fopen(path,'w');fprintf(f,'%d %d\n',target);fprintf(f,'%.17g\n',expected.');fclose(f);
   fprintf(manifest,'%s_b%d %d %d %d %d "%s" "%s"\n',name,b,h,w,target,strrep(folder,'\','/'),path);cases+=1;values+=numel(expected);
  endfor
 endfor
endfor
fclose(manifest);f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native general comparison binary/double-resize fixtures',...
 'source',source,'cases',cases,'double_values',values,'input_precision','single positive finite luminance','resize_precision','double binary imresize',...
 'layout','row-major uint32 input words/decimal double expected','padding','native imresize symmetric extension','DC','none')));fclose(f);
