# Diagnostic inverse192 scalar graphs from FFTW3.3.10,GPL-2.0-or-later.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$TwiddleFile)
$ErrorActionPreference='Stop';if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
$shader=@'
#version 450
#extension GL_ARB_gpu_shader_fp64 : require
layout(local_size_x=32) in;
layout(set=0,binding=0,std430) readonly buffer Input {dvec2 value[];} src;
layout(set=0,binding=1,std430) writeonly buffer Output {dvec2 value[];} dst;
layout(push_constant) uniform Push {uint n,batches,graph,fused;} p;
double FMA(double a,double b,double c){precise double t=a*b;precise double r=t+c;return r;}
double FNMS(double a,double b,double c){precise double t=a*b;precise double r=c-t;return r;}
double FMS(double a,double b,double c){precise double t=a*b;precise double r=t-c;return r;}
double FNMA(double a,double b,double c){precise double t=a*b;precise double r=-(t+c);return r;}
double sum_products(double a,double b,double c,double d,bool minusFirst){precise double t=a*b;precise double u=c*d;precise double r=minusFirst?u-t:u+t;return r;}
double divideCorrected(double a,double d){if(a==0.0lf)return a;precise double q=a/d;precise double r=fma(-q,d,a);precise double c=r/d;precise double result=q+c;return result;}
'@
function Body([string]$s,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}};throw 'Unbalanced graph'
}
$source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/fft256" 't2_16.c'
$functions=[regex]::Matches($source,'static void t2_16\(')
if($functions.Count -ne 2){throw 'Expected two scalar graphs'}
$powers=@([regex]::Matches($source.Substring($functions[1].Index),'TW_CEXP, 0, (\d+)')|ForEach-Object {[int]$_.Groups[1].Value})
if(($powers -join ',') -ne '1,3,9,15'){throw 'Unexpected twiddle powers'}
$body=Body $source ($source.IndexOf('{',$functions[1].Index))
$loop=[regex]::Match($body,'for\s*\(')
$ops=Body $body ($body.IndexOf('{',$loop.Index))
$ops=[regex]::Replace($ops,'\bE\b','precise double')
$ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','$1')
$ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
    param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
    "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
})
if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated graph'}
$shader+="`nvoid twiddle16(inout double ri[16],inout double ii[16],in double W[8]){`n"
foreach($c in [regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')){$shader+="const double $($c.Groups[1].Value)=$($c.Groups[2].Value)lf;`n"}
$shader+=$ops+"`n}`n"
function Literal([double]$v){$s=$v.ToString('R',[Globalization.CultureInfo]::InvariantCulture);if($s -notmatch '[.eE]'){$s+='.0'};return $s+'lf'}
$bytes=[IO.File]::ReadAllBytes((Resolve-Path $TwiddleFile));if($bytes.Length -ne 96*8){throw 'Invalid inverse192 twiddles'};$table=@();for($i=0;$i -lt 96;++$i){$table+=(Literal ([BitConverter]::ToDouble($bytes,$i*8)))}
$shader+="const double inverse192Twiddle[96]=double[96]("+($table -join ',')+");`n"
$source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/complex160" 'n1_12.c'
$f=[regex]::Matches($source,'static void n1_12\(');if($f.Count -ne 2){throw 'Expected two leaf graphs'}
$body=Body $source ($source.IndexOf('{',$f[1].Index));$loop=[regex]::Match($body,'for\s*\(')
$ops=Body $body ($body.IndexOf('{',$loop.Index))
$ops=[regex]::Replace($ops,'\bE\b','precise double')
$ops=[regex]::Replace($ops,'WS\((?:is|os),\s*(\d+)\)','$1')
$ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
    param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
    "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
})
if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated scalar leaf'}
$shader+="`nvoid leaf12(in double ri[12],in double ii[12],out double ro[12],out double io[12]){`n"
foreach($c in [regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')){$shader+="const double $($c.Groups[1].Value)=$($c.Groups[2].Value)lf;`n"}
$shader+=$ops+"`n}`n"

$shader+=@'
void main(){
 uint id=gl_GlobalInvocationID.x;if(id>=p.batches)return;uint matrix=id/192u,column=id%192u;
 double realPart[192],imagPart[192];
 for(uint j=0u;j<16u;++j){double ri[12],ii[12],ro[12],io[12];for(uint l=0u;l<12u;++l){dvec2 z=src.value[matrix*36864u+column*192u+j+16u*l];ri[l]=z.y;ii[l]=z.x;}leaf12(ri,ii,ro,io);for(uint k=0u;k<12u;++k){realPart[j+16u*k]=ro[k];imagPart[j+16u*k]=io[k];}}
 for(uint k=0u;k<12u;++k){double ri[16],ii[16],W[8];for(uint j=0u;j<16u;++j){ri[j]=realPart[j+16u*k];ii[j]=imagPart[j+16u*k];}for(uint w=0u;w<8u;++w)W[w]=inverse192Twiddle[k*8u+w];twiddle16(ri,ii,W);for(uint j=0u;j<16u;++j){dvec2 z=dvec2(ii[j],ri[j]);if(p.graph==1u)z=dvec2(divideCorrected(z.x,192.0lf),divideCorrected(z.y,192.0lf));dst.value[matrix*36864u+(k+12u*j)*192u+column]=z;}}
}
'@
[IO.File]::WriteAllText("$OutputDirectory/inverse192_double_axis.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"

