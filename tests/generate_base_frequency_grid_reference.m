1;
args=argv();out=args{1};assert(!exist(out,'dir'));assert(mkdir(out));addpath(genpath('C:/HDR-VDP-3_Ref'));assert(hdrvdp_version()==3.07);
f=fopen(fullfile(out,'grid.txt'),'w');assert(f>=0);
for shape={[10,12],[12,10]}
 hw=shape{1};grid=create_cycdeg_image(hw,3.75,false);fprintf(f,'%d %d %.17g\n',hw,3.75);fprintf(f,'%.17g\n',grid(:));
endfor
fclose(f);f=fopen(fullfile(out,'source_hash.txt'),'w');fprintf(f,'%s\n',hash('sha256',fileread(which('create_cycdeg_image'))));fclose(f);
