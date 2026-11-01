args=argv();
if numel(args)~=2,error('Usage: input.f64 fresh-output.f64');end
if exist(args{2},'file'),error('Fresh output required');end
f=fopen(args{1},'rb','ieee-le');if f<0,error('Input open failed');end
x=fread(f,Inf,'double=>double');fclose(f);
if isempty(x)||any(~isfinite(x))||any(x<=0),error('Invalid domain');end
% Same built-in logarithms used by the official HDR-VDP-3 Octave pipeline.
% This is a query oracle, not a run of the complete official metric.
y=[log(x)';log10(x)'];
f=fopen(args{2},'wb','ieee-le');if f<0,error('Output open failed');end
written=fwrite(f,y,'double');fclose(f);
if written~=2*numel(x),error('Output write failed');end
fprintf('%d query pairs exported;Octave %s\n',numel(x),version());
