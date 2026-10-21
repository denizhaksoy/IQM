# GPL-2.0-or-later FFTW arithmetic graphs; isolated residual16 diagnostic.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$s,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}};throw 'Unbalanced source'
}
$shader=@'
#version 450
// SPDX-License-Identifier: GPL-2.0-or-later; FFTW3.3.10, see COPYING and source notices.
layout(local_size_x=32) in;
layout(set=0,binding=0,std430) readonly buffer Input {vec2 value[];} src;
layout(set=0,binding=1,std430) writeonly buffer Output {float value[];} dst;
layout(push_constant) uniform Push {uint n,batches,graph,fused;} p;
float FMA(float a,float b,float c){precise float t=a*b;precise float r=t+c;return r;}
float FNMS(float a,float b,float c){precise float t=a*b;precise float r=c-t;return r;}
float FMS(float a,float b,float c){precise float t=a*b;precise float r=t-c;return r;}
float FNMA(float a,float b,float c){precise float t=a*b;precise float r=-(t+c);return r;}
'@
foreach($name in @('r2cf_16','n1_16')){
    $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/base16" "$name.c"
    $functions=[regex]::Matches($source,"static void $name\(")
    if($functions.Count -ne 2){throw 'Expected two scalar graphs'}
    $body=Body $source ($source.IndexOf('{',$functions[1].Index));$constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\(');$ops=Body $body ($body.IndexOf('{',$loop.Index))
    $ops=[regex]::Replace($ops,'\bE\b','precise float')
    $ops=[regex]::Replace($ops,'WS\((?:rs|csr|csi|is|os),\s*(\d+)\)','$1')
    if($name -eq 'r2cf_16'){
        $ops=[regex]::Replace($ops,'R0\[(\d+)\]','samples[2*$1]');$ops=[regex]::Replace($ops,'R1\[(\d+)\]','samples[2*$1+1]')
        $signature='void real16(in float samples[16],out float Cr[9],out float Ci[9])'
    }else{$signature='void complex16(in float ri[16],in float ii[16],out float ro[16],out float io[16])'}
    if($ops -match '\b(?:WS|R0|R1|INT|stride)\b'){throw 'Untranslated operation'}
    $shader+="`n$signature{`n";foreach($c in $constants){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
    if($name -eq 'r2cf_16'){$shader+="for(int k=0;k<9;++k){Cr[k]=0;Ci[k]=0;}`n"}
    $shader+=$ops+"`n}`n"
}
$shader+=[IO.File]::ReadAllText("$PSScriptRoot/math_reference/hypot_reference.glsl")
$shader+=@'
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;
    precise vec2 spectrum[256];
    for(uint x=0u;x<16u;++x){
        float samples[16],r[9],im[9];for(uint y=0u;y<16u;++y)samples[y]=src.value[batch*256u+y*16u+x].x;
        real16(samples,r,im);for(uint ky=0u;ky<9u;++ky)spectrum[ky*16u+x]=vec2(r[ky],im[ky]);
    }
    for(uint ky=0u;ky<9u;++ky){
        float ri[16],ii[16],ro[16],io[16];for(uint x=0u;x<16u;++x){ri[x]=spectrum[ky*16u+x].x;ii[x]=spectrum[ky*16u+x].y;}
        complex16(ri,ii,ro,io);for(uint kx=0u;kx<16u;++kx)spectrum[ky*16u+kx]=vec2(ro[kx],io[kx]);
    }
    for(uint ky=9u;ky<16u;++ky)for(uint kx=0u;kx<16u;++kx){vec2 z=spectrum[(16u-ky)*16u+(16u-kx)%16u];spectrum[ky*16u+kx]=vec2(z.x,-z.y);}
    for(uint i=0u;i<256u;++i){dst.value[batch*769u+2u*i]=spectrum[i].x;dst.value[batch*769u+2u*i+1u]=spectrum[i].y;}
    float maximum=0;uint selected=0;
    for(uint x=0u;x<16u;++x)for(uint y=0u;y<16u;++y){
        uint row=y*16u+x;vec2 z=spectrum[row];float magnitude=row==0u?0:finite_hypot(z.x,z.y);
        dst.value[batch*769u+512u+row]=magnitude;if(magnitude>maximum){maximum=magnitude;selected=x*16u+y;}
    }
    dst.value[batch*769u+768u]=float(selected);
}
'@
[IO.File]::WriteAllText("$OutputDirectory/base16.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
