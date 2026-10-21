# Isolated real256 first axis, translated from ordinary FFTW scalar graphs.
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
foreach($name in @('r2cf_8','r2cf_32','r2cfII_32','hc2cf_32')){
    $dir=if($name -eq 'r2cf_8'){'runtime'}else{'fft256'}
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
    if($name -eq 'hc2cf_32'){
        if($source -notmatch 'TW_FULL, 1, 32'){throw 'Unexpected full twiddle descriptor'}
        $ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','(16*$1)')
        $ops=[regex]::Replace($ops,'\b(Rp|Ip|Rm|Im)\[([^\]]+)\]','B[$1+$2]')
        $signature='void hc32(inout float B[258],int Rp,int Ip,int Rm,int Im,in float W[62])'
    }else{
        $size=if($name -eq 'r2cf_8'){8}else{32}
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
$table=@();for($k=1;$k -le 3;++$k){for($q=1;$q -le 31;++$q){
    $angle=2*[Math]::PI*$k*$q/256
    $table+=(Literal ([single][Math]::Cos($angle)));$table+=(Literal ([single][Math]::Sin($angle)))
}}
$shader+="const float twiddle[186]=float[186]("+($table -join ',')+");`n"
$shader+=@'
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=256u)return;
    float B[258];for(int i=0;i<258;++i)B[i]=0;
    for(int j=0;j<32;++j){
        float samples[8],r[5],im[5];
        for(int k=0;k<8;++k)samples[k]=src.value[batch*256u+uint(j+32*k)].x;
        r2cf_8(samples,r,im);
        for(int k=0;k<8;++k)B[(j/2)*16+2*k+j%2]=k<=4?r[k]:im[8-k];
    }
    float samples[32],r[17],im[17];
    for(int j=0;j<32;++j)samples[j]=B[(j/2)*16+j%2];
    r2cf_32(samples,r,im);
    for(int q=0;q<=16;++q){B[16*q]=r[q];B[16*q+1]=q==0||q==16?0:im[q];}
    for(int m=1;m<=3;++m){
        float W[62];for(int w=0;w<62;++w)W[w]=twiddle[(m-1)*62+w];
        hc32(B,2*m,2*m+1,2*(8-m),2*(8-m)+1,W);
    }
    for(int j=0;j<32;++j)samples[j]=B[(j/2)*16+8+j%2];
    r2cfII_32(samples,r,im);
    for(int q=0;q<16;++q){B[16*q+8]=r[q];B[16*q+9]=im[q];}
    for(int k=0;k<129;++k)dst.value[batch*129u+uint(k)]=vec2(B[2*k],B[2*k+1]);
}
'@
[IO.File]::WriteAllText("$OutputDirectory/forward256_axis.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item (Join-Path $PSScriptRoot 'fftw_reference/COPYING') "$OutputDirectory/COPYING"
