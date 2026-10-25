# Isolated real192 first axis, translated from ordinary FFTW scalar graphs.
# Derived from generate_fft256_forward_axis_shader.ps1; FFTW sources are
# GPL-2.0-or-later (fftw_reference/COPYING). Output is diagnostic only.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
. (Join-Path $PSScriptRoot 'fftw_reference/read_runtime_source.ps1')
function Body([string]$text,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $text.Length;++$i){
        if($text[$i] -eq '{'){++$depth}
        if($text[$i] -eq '}'){--$depth;if(!$depth){return $text.Substring($opening+1,$i-$opening-1)}}
    };throw 'Unbalanced graph'
}
$shader=@'
#version 450
layout(local_size_x=32) in;
layout(set=0,binding=0,std430) readonly buffer Input {vec2 value[];} src;
layout(set=0,binding=1,std430) writeonly buffer Output {vec2 value[];} dst;
layout(push_constant) uniform Push {uint n,batches,graph,fused;} p;
float FMA(float a,float b,float c){precise float t=a*b;precise float r=t+c;return r;}
float FNMS(float a,float b,float c){precise float t=a*b;precise float r=c-t;return r;}
float FMS(float a,float b,float c){precise float t=a*b;precise float r=t-c;return r;}
float sum_products(float a,float b,float c,float d,bool minusFirst){precise float t=a*b;precise float u=c*d;precise float r=minusFirst?u-t:u+t;return r;}
'@
foreach($name in @('r2cf_8','r2cfII_8','r2cf_12','r2cfII_12','hc2cf_8')){
    $dir=if($name -eq 'r2cf_8'){'runtime'}else{'real192'}
    $source=Read-IQMFFTWSource (Join-Path $PSScriptRoot "fftw_reference/$dir") "$name.c"
    $functions=[regex]::Matches($source,"static void $name\(")
    if($functions.Count -ne 2){throw 'Expected ordinary and FMA graphs'}
    $body=Body $source ($source.IndexOf('{',$functions[1].Index))
    $constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\(')
    $ops=Body $body ($body.IndexOf('{',$loop.Index))
    $ops=[regex]::Replace($ops,'\bE\b','precise float')
    $ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
        param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
        "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
    })
    if($name -eq 'hc2cf_8'){
        if($source -notmatch 'TW_FULL, 1, 8'){throw 'Unexpected full twiddle descriptor'}
        $ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','(48*$1)')
        $ops=[regex]::Replace($ops,'\b(Rp|Ip|Rm|Im)\[([^\]]+)\]','B[$1+$2]')
        $signature='void hc8(inout float B[194],int Rp,int Ip,int Rm,int Im,in float W[14])'
    }else{
        $size=if($name -match '_8$'){8}else{12}
        $outSize=1+$size/2
        $ops=[regex]::Replace($ops,'WS\((?:rs|csr|csi),\s*(\d+)\)','$1')
        $ops=[regex]::Replace($ops,'R0\[(\d+)\]','samples[2*$1]')
        $ops=[regex]::Replace($ops,'R1\[(\d+)\]','samples[2*$1+1]')
        $signature="void $name(in float samples[$size],out float Cr[$outSize],out float Ci[$outSize])"
    }
    if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated scalar code'}
    $shader+="`n$signature{`n"
    foreach($constant in $constants){$shader+="const float $($constant.Groups[1].Value)=$($constant.Groups[2].Value);`n"}
    $shader+=$ops+"`n}`n"
}
function Literal([single]$value){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($value),0)}
$table=@();for($m=1;$m -le 11;++$m){for($q=1;$q -le 7;++$q){
    $phase=($m*$q)%192;$angle=2*[Math]::PI*$phase/192
    $re=[single][Math]::Cos($angle);$im=[single][Math]::Sin($angle)
    if($phase -eq 48){$re=0;$im=1}
    $table+=(Literal $re);$table+=(Literal $im)
}}
$shader+="const float twiddle[154]=float[154]("+($table -join ',')+");`n"
$shader+=@'
void real24(in float samples[24],out float hc[24]){
    float sum[12],difference[12],r[7],im[7];
    // Twelve real2 leaves; hc2hc radix12 has no interior m for inner size2.
    for(int j=0;j<12;++j){precise float a=samples[j]+samples[j+12];precise float b=samples[j]-samples[j+12];sum[j]=a;difference[j]=b;}
    r2cf_12(sum,r,im);
    for(int q=0;q<=6;++q){hc[2*q]=r[q];if(q>0&&q<6)hc[24-2*q]=im[q];}
    r2cfII_12(difference,r,im);
    for(int q=0;q<6;++q){hc[2*q+1]=r[q];hc[23-2*q]=im[q];}
}
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;
    float B[194];for(int i=0;i<194;++i)B[i]=0;
    for(int j=0;j<8;++j){
        float samples[24],hc[24];for(int k=0;k<24;++k)samples[k]=src.value[batch*192u+uint(j+8*k)].x;
        real24(samples,hc);for(int k=0;k<24;++k)B[(j/2)*48+2*k+j%2]=hc[k];
    }
    float samples[8],r[5],im[5];
    for(int j=0;j<8;++j)samples[j]=B[(j/2)*48+j%2];
    r2cf_8(samples,r,im);
    for(int q=0;q<=4;++q){B[48*q]=r[q];B[48*q+1]=q==0||q==4?0:im[q];}
    for(int m=1;m<=11;++m){
        float W[14];for(int w=0;w<14;++w)W[w]=twiddle[(m-1)*14+w];
        hc8(B,2*m,2*m+1,2*(24-m),2*(24-m)+1,W);
    }
    for(int j=0;j<8;++j)samples[j]=B[(j/2)*48+24+j%2];
    r2cfII_8(samples,r,im);
    for(int q=0;q<4;++q){B[48*q+24]=r[q];B[48*q+25]=im[q];}
    for(int k=0;k<97;++k)dst.value[batch*97u+uint(k)]=vec2(B[2*k],B[2*k+1]);
}
'@
[IO.File]::WriteAllText("$OutputDirectory/forward192_axis.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
