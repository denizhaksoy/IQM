% Deterministic RGB16 PQ synthetic fixtures; not a paper dataset substitute.
args=argv();assert(numel(args)==1||numel(args)==2);out=args{1};assert(~exist(out,'dir'));assert(mkdir(out));
spec={
 'landscape',96,80,96,80,62.729130063099447;
 'portrait',80,96,80,96,62.729130063099447;
 'odd_landscape',127,95,127,95,62.729130063099447;
 'odd_portrait',95,127,95,127,62.729130063099447;
 'ppd26',128,128,128,128,26;
 'ppd128',128,128,128,128,128;
 'ppd256',128,128,128,128,256;
 'small_portrait',17,33,17,33,32;
 'small_landscape',33,17,33,17,128;
 'resize_up',31,47,17,29,62.729130063099447;
 'resize_down',47,31,59,41,62.729130063099447;
 'identical',96,80,96,80,62.729130063099447;
 'deep7',256,256,256,256,256;
 'deep8',512,512,512,512,256
};
if numel(args)==2
 selection=strsplit(args{2},',');assert(numel(unique(selection))==numel(selection));
 assert(all(ismember(selection,spec(:,1))));spec=spec(ismember(spec(:,1),selection),:);
end
cases=struct([]);
for k=1:rows(spec)
 name=spec{k,1};w=spec{k,2};h=spec{k,3};tw=spec{k,4};th=spec{k,5};ppd=spec{k,6};
 files=cell(1,2);
 for side=1:2
  if side==1,iw=w;ih=h;else,iw=tw;ih=th;end
  [x,y]=meshgrid(0:iw-1,0:ih-1);image=zeros(ih,iw,3,'uint16');
  for c=1:3
   codes=mod((431+56*c)*x+(127+29*c)*y+503*c+3000*mod(floor(x/7)+floor(y/5),2),65536);
   if side==2&&~strcmp(name,'identical')
    codes=min(max(codes+(mod(x+3*y+c,17)==0).*(500+73*c),0),65535);
   end
   image(:,:,c)=uint16(codes);
  end
  if side==1,label='reference';else,label='test';end
  files{side}=fullfile(out,[name '_' label '.png']);imwrite(image,files{side});
  check=imread(files{side});assert(isa(check,'uint16')&&isequal(size(check),[ih iw 3]));assert(isequal(check,image));
 end
 cases(k)=struct('name',name,'reference',files{1},'test',files{2},'width',w,'height',h,'test_width',tw,'test_height',th,'ppd',ppd,...
  'reference_sha256',hash('sha256',fileread(files{1})),'test_sha256',hash('sha256',fileread(files{2})));
end
result=struct('scope','deterministic RGB16 PQ stress tests, not original paper cohort','encoding','pq-bt2020-rgb16','cases',cases);
f=fopen(fullfile(out,'fixtures.json'),'wt');assert(f>=0);fprintf(f,'%s\n',jsonencode(result));fclose(f);
