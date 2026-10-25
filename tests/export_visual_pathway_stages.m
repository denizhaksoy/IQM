1;
function write_values(path,a)
 assert(isreal(a)&&all(isfinite(a(:))));f=fopen(path,'w');assert(f>=0);fprintf(f,'%d %d\n',rows(a),columns(a));fprintf(f,'%.17g\n',a.');fclose(f);
endfunction
args=argv();assert(numel(args)==3);out=args{1};source=args{2};previous=args{3};newFixture=strcmp(previous,'--new-fixture');assert(!exist(out,'dir'));assert(mkdir(out));records={};
for shape={[80,96],[96,80]}
 hw=shape{1};tag=sprintf('h%d_w%d',hw(1),hw(2));loaded=load(fullfile(source,tag,'trace.mat'));if !newFixture,old=load(fullfile(previous,tag,'trace.mat'));endif;t=loaded.trace;
 if !newFixture,for field={'P_reference','P_test','log_La'}
  n=field{1};assert(isequal(typecast(t.(n)(:),'uint32'),typecast(old.trace.(n)(:),'uint32')));
 endfor;endif
 folder=fullfile(out,tag);assert(mkdir(folder));
 for label={'reference','test'}
  n=label{1};v=t.(['visual_' n]);assert(isequal(v.P,t.(['P_' n])));
  fields={'retinal','matrix','adaptation_input','adaptation','pupil','area','cones','rods','P'};classes=struct();
  for k=1:numel(fields),field=fields{k};a=v.(field);write_values(fullfile(folder,[n '_' field '.txt']),a);classes.(field)=class(a);endfor
  for k=1:4
   write_values(fullfile(folder,sprintf('%s_LMSR_before%d.txt',n,k)),v.LMSR_before(:,:,k));
   write_values(fullfile(folder,sprintf('%s_LMSR_after%d.txt',n,k)),v.LMSR_after(:,:,k));
  endfor
  for k=1:3,write_values(fullfile(folder,sprintf('%s_JND_channel%d.txt',n,k)),v.channels(:,:,k));endfor
  for k=1:2
   write_values(fullfile(folder,sprintf('%s_pn_Y%d.txt',n,k)),v.pn.Y{k}(:));
   write_values(fullfile(folder,sprintf('%s_pn_JND%d.txt',n,k)),v.pn.jnd{k}(:));
  endfor
  records{end+1}=struct('tag',tag,'label',n,'classes',classes,'LMSR_class',class(v.LMSR_before),'channels_class',class(v.channels));
 endfor
endfor
f=fopen(fullfile(out,'metadata.json'),'w');fputs(f,jsonencode(struct('scope','Native upstream visual-pathway reference stages; no GPU claim',...
 'source',source,'previous_trace',previous,'records',{records},'prior_P_and_log_bit_identity',!newFixture,'new_fixture',newFixture,...
 'ppd',30,'age',24,'layout','row-major decimal17digits; per-channel LMSR/JND maps',...
 'DC','no new removal; captures before SPYR; P andlog maps match earlier traces')));fclose(f);
f=fopen(fullfile(out,'PASS.txt'),'w');if newFixture,fputs(f,'Four new-fixture native upstream paths exported; source trace separately verifies stock/traced score identity.');else,fputs(f,'Four native upstream visual paths exported; P/log maps bit-identical to prior accepted traces.');endif;fclose(f);
