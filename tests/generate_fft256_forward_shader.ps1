# Isolated two-axis forward256 experiment; FFTW GPL-2.0-or-later graphs.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft256_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/real_axis"
# Defines Body and the source loader in this script's scope.
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$text,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $text.Length;++$i){
        if($text[$i] -eq '{'){++$depth}
        if($text[$i] -eq '}'){--$depth;if(!$depth){return $text.Substring($opening+1,$i-$opening-1)}}
    };throw 'Unbalanced graph'
}
& "$PSScriptRoot/generate_fft_codelet_shader.ps1" -OutputDirectory "$OutputDirectory/complex_leaves"
$base=[IO.File]::ReadAllText("$OutputDirectory/complex_leaves/fft128.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()'))
$axis=[IO.File]::ReadAllText("$OutputDirectory/real_axis/forward256_axis.comp")
$shader+=$axis.Substring($axis.IndexOf('void r2cf_8')).Replace('void main()','void firstAxis()')
$source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/fft256" 't3fv_16.c'
$functions=[regex]::Matches($source,'static void t3fv_16\(')
if($functions.Count -ne 2){throw 'Expected two SIMD graph variants'}
$powers=@([regex]::Matches($source.Substring($functions[1].Index),'VTW\(0, (\d+)\)')|ForEach-Object {[int]$_.Groups[1].Value})
if($powers.Count -ne 4){throw 'Unexpected SIMD descriptor'}
$body=Body $source ($source.IndexOf('{',$functions[1].Index))
$constants=[regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')
$loop=[regex]::Match($body,'for\s*\(')
$ops=Body $body ($body.IndexOf('{',$loop.Index))
$ops=[regex]::Replace($ops,'\bV\b','precise vec2')
$ops=[regex]::Replace($ops,'LDW\(&\(W\[0\]\)\)','W[0]')
$ops=[regex]::Replace($ops,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m)'W['+([int]$m.Groups[1].Value/2)+']'})
$ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','$1')
$ops=[regex]::Replace($ops,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
$ops=[regex]::Replace($ops,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1]=$2;')
$ops=[regex]::Replace($ops,'LDK\((\w+)\)','vec2($1)')
if($ops -match '\b(?:LD|ST|LDW|WS|TWVL)\b'){throw 'Untranslated SIMD code'}
$shader+="`nvoid forwardTwiddle16(inout vec2 x[16],in vec2 W[4]){`n"
foreach($c in $constants){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
$shader+=$ops+"`n}`n"
function Literal([single]$v){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($v),0)}
$table=@();for($k=0;$k -lt 16;++$k){foreach($power in $powers){
    $phase=($k*$power)%256;$angle=2*[Math]::PI*$phase/256
    $re=[single][Math]::Cos($angle);$im=[single][Math]::Sin($angle)
    if($phase -eq 0){$re=1;$im=0};if($phase -eq 64){$re=0;$im=1}
    if($phase -eq 128){$re=-1;$im=0};if($phase -eq 192){$re=0;$im=-1}
    $table+='vec2('+(Literal $re)+','+(Literal $im)+')'
}}
$shader+="const vec2 forward256Twiddle[64]=vec2[64]("+($table -join ',')+");`n"
$shader+=@'
void main(){
    if(p.graph==3u){firstAxis();return;}
    uint batch=gl_GlobalInvocationID.x;if(batch>=129u)return;
    vec2 intermediate[256];
    for(uint j=0u;j<16u;++j){
        vec2 x[16],y[16];
        for(uint l=0u;l<16u;++l)x[l]=src.value[(j+16u*l)*129u+batch];
        forward16(x,y);
        for(uint k=0u;k<16u;++k)intermediate[j+16u*k]=y[k];
    }
    for(uint k=0u;k<16u;++k){
        vec2 x[16],W[4];for(uint j=0u;j<16u;++j)x[j]=intermediate[j+16u*k];
        for(uint w=0u;w<4u;++w)W[w]=forward256Twiddle[k*4u+w];
        forwardTwiddle16(x,W);
        for(uint j=0u;j<16u;++j){
            uint frequency=k+16u*j;dst.value[frequency*256u+batch]=x[j];
            if(batch>0u&&batch<128u)dst.value[((256u-frequency)%256u)*256u+256u-batch]=vec2(x[j].x,-x[j].y);
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/forward256.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
