# Isolated unnormalized inverse192, FFTW3.3.10 GPL-2.0-or-later graphs.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
& "$PSScriptRoot/generate_fft_codelet_shader.ps1" -OutputDirectory "$OutputDirectory/helpers"
$base=[IO.File]::ReadAllText("$OutputDirectory/helpers/fft128.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()'))
$begin=$shader.IndexOf('float FMA(');$end=$shader.IndexOf('void graph8_0')
$helpers=@'
float FMA(float a,float b,float c){precise float t=a*b;precise float r=t+c;return r;}
float FNMS(float a,float b,float c){precise float t=a*b;precise float r=c-t;return r;}
float FMS(float a,float b,float c){precise float t=a*b;precise float r=t-c;return r;}
float FNMA(float a,float b,float c){precise float t=a*b;precise float r=-(t+c);return r;}
float sum_products(float a,float b,float c,float d,bool subtractFirst){precise float t=a*b;precise float u=c*d;precise float r=subtractFirst?u-t:u+t;return r;}
'@
$shader=$shader.Substring(0,$begin)+$helpers+"`n"+$shader.Substring($end)
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
$ops=[regex]::Replace($ops,'\bE\b','precise float')
$ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','$1')
$ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
    param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
    "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
})
if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated graph'}
$shader+="`nvoid twiddle16(inout float ri[16],inout float ii[16],in float W[8]){`n"
foreach($c in [regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
$shader+=$ops+"`n}`n"
function Literal([single]$v){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($v),0)}
$table=@();for($k=0;$k -lt 12;++$k){foreach($power in $powers){
    $phase=($k*$power)%192;$angle=2*[Math]::PI*$phase/192
    $re=[single][Math]::Cos($angle);$im=[single][Math]::Sin($angle)
    if($phase -eq 0){$re=1;$im=0};if($phase -eq 48){$re=0;$im=1}
    if($phase -eq 96){$re=-1;$im=0};if($phase -eq 144){$re=0;$im=-1}
    $table+=(Literal $re);$table+=(Literal $im)
}}
$shader+="const float inverse192Twiddle[96]=float[96]("+($table -join ',')+");`n"
$source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/complex160" 'n1_12.c'
$f=[regex]::Matches($source,'static void n1_12\(');if($f.Count -ne 2){throw 'Expected two leaf graphs'}
$body=Body $source ($source.IndexOf('{',$f[1].Index));$loop=[regex]::Match($body,'for\s*\(')
$ops=Body $body ($body.IndexOf('{',$loop.Index))
$ops=[regex]::Replace($ops,'\bE\b','precise float')
$ops=[regex]::Replace($ops,'WS\((?:is|os),\s*(\d+)\)','$1')
$ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
    param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
    "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
})
if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated scalar leaf'}
$shader+="`nvoid leaf12(in float ri[12],in float ii[12],out float ro[12],out float io[12]){`n"
foreach($c in [regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
$shader+=$ops+"`n}`n"
$shader+=@'
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches||p.n!=192u)return;
    float realPart[192],imagPart[192];
    for(uint j=0u;j<16u;++j){
        float ri[12],ii[12],ro[12],io[12];
        for(uint l=0u;l<12u;++l){vec2 v=src.value[batch*192u+j+16u*l];ri[l]=v.y;ii[l]=v.x;}
        leaf12(ri,ii,ro,io);
        for(uint k=0u;k<12u;++k){realPart[j+16u*k]=ro[k];imagPart[j+16u*k]=io[k];}
    }
    for(uint k=0u;k<12u;++k){
        float ri[16],ii[16],W[8];
        for(uint j=0u;j<16u;++j){ri[j]=realPart[j+16u*k];ii[j]=imagPart[j+16u*k];}
        for(uint w=0u;w<8u;++w)W[w]=inverse192Twiddle[k*8u+w];
        twiddle16(ri,ii,W);
        for(uint j=0u;j<16u;++j)dst.value[(k+12u*j)*p.batches+batch]=vec2(ii[j],ri[j]);
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/inverse192_axis.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
