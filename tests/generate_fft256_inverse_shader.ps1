# Isolated inverse256 experiment; GPL-2.0-or-later FFTW graphs.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Use a fresh output directory'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
. (Join-Path $PSScriptRoot 'fftw_reference/read_runtime_source.ps1')
& (Join-Path $PSScriptRoot 'generate_fft_codelet_shader.ps1') -OutputDirectory "$OutputDirectory/leaf128"
$base=[IO.File]::ReadAllText("$OutputDirectory/leaf128/fft128.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()'))
function Body([string]$text,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $text.Length;++$i){
        if($text[$i] -eq '{'){++$depth}
        if($text[$i] -eq '}'){--$depth;if($depth -eq 0){return $text.Substring($opening+1,$i-$opening-1)}}
    };throw 'Unbalanced codelet'
}
$source=Read-IQMFFTWSource (Join-Path $PSScriptRoot 'fftw_reference/fft256') 't2_16.c'
$functions=[regex]::Matches($source,'static void t2_16\(')
if($functions.Count -ne 2){throw 'Expected2 scalar twiddle graphs'}
$scalarPowers=@([regex]::Matches($source.Substring($functions[1].Index),'TW_CEXP, 0, (\d+)')|ForEach-Object {[int]$_.Groups[1].Value})
if($scalarPowers.Count -ne 4){throw 'Unexpected scalar twiddle descriptor'}
$body=Body $source ($source.IndexOf('{',$functions[1].Index))
$constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
$loop=[regex]::Match($body,'for\s*\(')
$operations=Body $body ($body.IndexOf('{',$loop.Index))
$operations=[regex]::Replace($operations,'\bE\b','precise float')
$operations=[regex]::Replace($operations,'WS\(rs,\s*(\d+)\)','$1')
$operations=[regex]::Replace($operations,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
    param($m);$subtract=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
    return "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$subtract)"
})
if($operations -match '\b(?:WS|INT|stride)\b'){throw 'Untranslated scalar code'}
$shader+="`nvoid twiddle16(inout float ri[16],inout float ii[16],in float W[8]){`n"
foreach($constant in $constants){$shader+="const float $($constant.Groups[1].Value)=$($constant.Groups[2].Value);`n"}
$shader+=$operations+"`n}`n"
$source=Read-IQMFFTWSource (Join-Path $PSScriptRoot 'fftw_reference/fft256') 't3bv_32.c'
$functions=[regex]::Matches($source,'static void t3bv_32\(')
if($functions.Count -ne 2){throw 'Expected2 SIMD twiddle graphs'}
$simdPowers=@([regex]::Matches($source.Substring($functions[1].Index),'VTW\(0, (\d+)\)')|ForEach-Object {[int]$_.Groups[1].Value})
if($simdPowers.Count -ne 4){throw 'Unexpected SIMD twiddle descriptor'}
$body=Body $source ($source.IndexOf('{',$functions[1].Index))
$constants=[regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')
$loop=[regex]::Match($body,'for\s*\(')
$operations=Body $body ($body.IndexOf('{',$loop.Index))
$operations=[regex]::Replace($operations,'\bV\b','precise vec2')
$operations=[regex]::Replace($operations,'LDW\(&\(W\[0\]\)\)','W[0]')
$operations=[regex]::Replace($operations,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m)'W['+([int]$m.Groups[1].Value/2)+']'})
$operations=[regex]::Replace($operations,'WS\(rs,\s*(\d+)\)','$1')
$operations=[regex]::Replace($operations,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
$operations=[regex]::Replace($operations,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1]=$2;')
$operations=[regex]::Replace($operations,'LDK\((\w+)\)','vec2($1)')
if($operations -match '\b(?:LD|ST|LDW|WS|TWVL)\b'){throw 'Untranslated SIMD code'}
$shader+="`nvoid inverseTwiddle32(inout vec2 x[32],in vec2 W[4]){`n"
foreach($constant in $constants){$shader+="const float $($constant.Groups[1].Value)=$($constant.Groups[2].Value);`n"}
$shader+=$operations+"`n}`n"
function Literal([single]$value){return ('uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($value),0))}
$table=@();for($k=0;$k -lt 16;++$k){foreach($power in $scalarPowers){
    $angle=2*[Math]::PI*$k*$power/256
    $table+=(Literal ([single][Math]::Cos($angle)));$table+=(Literal ([single][Math]::Sin($angle)))
}}
$shader+="const float twiddle256[128]=float[128]("+($table -join ',')+");`n"
$table=@();for($k=0;$k -lt 8;++$k){foreach($power in $simdPowers){
    $phase=($k*$power)%256;$angle=2*[Math]::PI*$phase/256
    $real=[single][Math]::Cos($angle);$imag=[single][Math]::Sin($angle)
    if($phase -eq 0){$real=1;$imag=0};if($phase -eq 64){$real=0;$imag=1}
    if($phase -eq 128){$real=-1;$imag=0};if($phase -eq 192){$real=0;$imag=-1}
    $table+='vec2('+(Literal $real)+','+(Literal $imag)+')'
}}
$shader+="const vec2 simd256[32]=vec2[32]("+($table -join ',')+");`n"
$shader+=@'
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches||p.n!=256u)return;
    if(p.graph==3u){
        float realPart[256],imagPart[256];
        for(uint j=0u;j<16u;++j){
            float ri[16],ii[16],ro[16],io[16];
            for(uint l=0u;l<16u;++l){vec2 v=src.value[batch*256u+j+16u*l];ri[l]=v.y;ii[l]=v.x;}
            graph16_1(ri,ii,ro,io);
            for(uint k=0u;k<16u;++k){realPart[j+16u*k]=ro[k];imagPart[j+16u*k]=io[k];}
        }
        for(uint k=0u;k<16u;++k){
            float ri[16],ii[16],W[8];
            for(uint j=0u;j<16u;++j){ri[j]=realPart[j+16u*k];ii[j]=imagPart[j+16u*k];}
            for(uint w=0u;w<8u;++w)W[w]=twiddle256[k*8u+w];
            twiddle16(ri,ii,W);
            for(uint j=0u;j<16u;++j)dst.value[(k+16u*j)*256u+batch]=vec2(ii[j],ri[j]);
        }
    }else if(p.graph==4u){
        vec2 intermediate[256];
        for(uint j=0u;j<32u;++j){
            float ri[8],ii[8],ro[8],io[8];
            for(uint l=0u;l<8u;++l){vec2 v=src.value[batch*256u+j+32u*l];ri[l]=v.y;ii[l]=v.x;}
            graph8_1(ri,ii,ro,io);
            for(uint k=0u;k<8u;++k)intermediate[j+32u*k]=vec2(io[k],ro[k]);
        }
        for(uint k=0u;k<8u;++k){
            vec2 x[32],W[4];for(uint j=0u;j<32u;++j)x[j]=intermediate[j+32u*k];
            for(uint w=0u;w<4u;++w)W[w]=simd256[k*4u+w];
            inverseTwiddle32(x,W);
            for(uint j=0u;j<32u;++j)dst.value[(k+8u*j)*256u+batch]=x[j]/65536.0;
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/inverse256.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item (Join-Path $PSScriptRoot 'fftw_reference/COPYING') "$OutputDirectory/COPYING"
