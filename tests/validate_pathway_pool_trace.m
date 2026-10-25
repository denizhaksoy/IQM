% Compare experimental GPU pool dumps directly to full-metric trace.self.
1;
args=argv();assert(numel(args)==3);source=args{1};gpu=args{2};out=args{3};assert(!exist(out,'dir'));assert(mkdir(out));
count=0;values=0;
for shape={[80,96],[96,80]}
 hw=shape{1};loaded=load(fullfile(source,sprintf('h%d_w%d',hw(1),hw(2)),'trace.mat'));t=loaded.trace;
 for b=1:numel(t.log_resized)
  expected=t.masking{b,1}.self;assert(isa(expected,'single'));
  f=fopen(fullfile(gpu,sprintf('case%d.u32',count)),'rb','ieee-le');assert(f>=0);actual=fread(f,Inf,'*uint32');fclose(f);
  n=numel(expected);assert(numel(actual)==2*n);
  assert(isequal(actual(n+1:end),typecast(reshape(expected.',[],1),'uint32')));
  values+=n;count+=1;
 endfor
endfor
assert(count==10&&values==35760);
f=fopen(fullfile(out,'PASS.txt'),'w');fputs(f,'Ten experimental GPU pooled masks match35760 full native metric trace.self words bitwise.');fclose(f);
fprintf('Full native mask trace:cases=%d bitwise_values=%d mismatches=0\n',count,values);
