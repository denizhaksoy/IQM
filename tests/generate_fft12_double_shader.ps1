# Diagnostic Double graph translation; derived FFTW GPL-2.0-or-later codelets.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$text,[int]$start){$depth=0;for($i=$start;$i -lt $text.Length;++$i){if($text[$i] -eq '{'){++$depth};if($text[$i] -eq '}'){--$depth;if(!$depth){return $text.Substring($start+1,$i-$start-1)}}};throw 'Unbalanced codelet'}
$shader=@'
#version 450
#extension GL_ARB_gpu_shader_fp64 : require
layout(local_size_x=16) in;
layout(set=0,binding=0,std430) readonly buffer Input {dvec2 value[];} src;
layout(set=0,binding=1,std430) buffer Output {dvec2 value[];} dst;
layout(push_constant) uniform Push {uint stage,batches,reserved0,reserved1;} p;
double FMA(double a,double b,double c){precise double t=a*b;precise double r=t+c;return r;}
double FNMS(double a,double b,double c){precise double t=a*b;precise double r=c-t;return r;}
double FMS(double a,double b,double c){precise double t=a*b;precise double r=t-c;return r;}
'@
foreach($name in 'r2cf_12','n1_12'){
 $dir=if($name -eq 'r2cf_12'){'real192'}else{'complex160'}
 $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/$dir" "$name.c"
 $matches=[regex]::Matches($source,"static void $name\(");if($matches.Count -ne 2){throw 'Expected ordinary/FMA source variants'}
 $body=Body $source ($source.IndexOf('{',$matches[1].Index));$constants=[regex]::Matches($body,'DK\((\w+),\s*([^\)]+)\)');$loop=[regex]::Match($body,'for\s*\(');$ops=Body $body ($body.IndexOf('{',$loop.Index))
 $ops=[regex]::Replace($ops,'\bE\b','precise double');$ops=[regex]::Replace($ops,'WS\((?:rs|csr|csi|is|os),\s*(\d+)\)','$1')
 if($name -eq 'r2cf_12'){
  $ops=[regex]::Replace($ops,'R0\[(\d+)\]','samples[2*$1]');$ops=[regex]::Replace($ops,'R1\[(\d+)\]','samples[2*$1+1]')
  $signature='void r2cf_12(in double samples[12],out double Cr[7],out double Ci[7])'
 }else{$signature='void n1_12(in double ri[12],in double ii[12],out double ro[12],out double io[12])'}
 if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated codelet token'}
 $shader+="`n$signature{`n";foreach($constant in $constants){$shader+="const double $($constant.Groups[1].Value)=$($constant.Groups[2].Value.Trim())lf;`n"}
 if($name -eq 'r2cf_12'){$shader+="for(int j=0;j<7;++j){Cr[j]=0.0lf;Ci[j]=0.0lf;}`n"}
 $shader+=$ops+"`n}`n"
}
$shader+=@'
void main(){
 uint id=gl_GlobalInvocationID.x,batch=id/12u,line=id%12u;if(batch>=p.batches)return;
 uint base=batch*144u;
 if(p.stage==0u){double samples[12],Cr[7],Ci[7];for(uint j=0u;j<12u;++j)samples[j]=src.value[base+line*12u+j].x;r2cf_12(samples,Cr,Ci);for(uint j=0u;j<7u;++j)dst.value[base+line*12u+j]=dvec2(Cr[j],Ci[j]);}
 else if(p.stage==1u){if(line>=7u)return;double ri[12],ii[12],ro[12],io[12];for(uint j=0u;j<12u;++j){dvec2 z=src.value[base+j*12u+line];ri[j]=z.x;ii[j]=z.y;}n1_12(ri,ii,ro,io);for(uint j=0u;j<12u;++j){dst.value[base+j*12u+line]=dvec2(ro[j],io[j]);if(line>0u&&line<6u)dst.value[base+((12u-j)%12u)*12u+12u-line]=dvec2(ro[j],-io[j]);}}
 else if(line==0u){dst.value[base]=dvec2(0.0lf);bool realSpectrum=true;for(uint j=0u;j<144u;++j)if(dst.value[base+j].y!=0.0lf)realSpectrum=false;if(realSpectrum)for(uint j=0u;j<144u;++j)dst.value[base+j].y=0.0lf;}
}
'@
[IO.File]::WriteAllText("$OutputDirectory/fft12_double.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
