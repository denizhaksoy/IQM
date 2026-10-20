# GPL-2.0-or-later FFTW3.3.10 translation for the opt-in diagnostic base FFT.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$FFTWArchive='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$archive=$FFTWArchive
if($archive -and (Get-FileHash $archive).Hash -ne '56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467') { throw 'FFTW archive hash mismatch' }
. (Join-Path $PSScriptRoot 'fftw_reference/read_runtime_source.ps1')
if(Test-Path -LiteralPath $OutputDirectory) { throw 'Output directory must be new' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
if($archive){
    $realSource=(& tar -xOf $archive fftw-3.3.10/rdft/scalar/r2cf/r2cf_8.c) -join "`n"
    if($LASTEXITCODE -ne 0) { throw 'FFTW source extraction failed' }
    $complexSource=(& tar -xOf $archive fftw-3.3.10/dft/scalar/codelets/n1_8.c) -join "`n"
    if($LASTEXITCODE -ne 0) { throw 'FFTW source extraction failed' }
}else{
    $realSource=Read-IQMFFTWSource (Join-Path $PSScriptRoot 'fftw_reference/runtime') 'r2cf_8.c'
    $complexSource=Read-IQMFFTWSource (Join-Path $PSScriptRoot 'fftw_reference/runtime') 'n1_8.c'
}
function Body([string]$source,[int]$opening) {
    $depth=0
    for($i=$opening;$i -lt $source.Length;++$i) {
        if($source[$i] -eq '{') {++$depth}
        if($source[$i] -eq '}') {--$depth; if($depth -eq 0){return $source.Substring($opening+1,$i-$opening-1)}}
    }
    throw 'Unbalanced source braces'
}
$shader=@'
#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// FFTW3.3.10 graphs; original notices and COPYING preserved beside this shader.
// Copyright (c)2003,2007-14 Matteo Frigo and Massachusetts Institute of Technology.
// Modified2026 for isolated IQM numerical tests. Not a production backend.
layout(local_size_x=32) in;
layout(constant_id=31) const uint base_fft_abi=81u;
layout(set=0,binding=0,std430) readonly buffer Input {vec2 value[];} src;
layout(set=0,binding=1,std430) writeonly buffer Output {vec2 value[];} dst;
layout(push_constant) uniform Parameters {uint n,batches,graph,fused;} p;
float FMA(float a,float b,float c){precise float product=a*b;return product+c;}
float FNMS(float a,float b,float c){precise float product=a*b;return c-product;}
float FMS(float a,float b,float c){precise float product=a*b;return product-c;}
float FNMA(float a,float b,float c){precise float product=a*b;return -(product+c);}
'@
foreach($kind in @('real','complex')) {
    $source=$complexSource; $name='n1_8'; $signature='void complex8(in float ri[8],in float ii[8],out float ro[8],out float io[8])'
    if($kind -eq 'real') {$source=$realSource; $name='r2cf_8'; $signature='void real8(in float samples[8],out float Cr[5],out float Ci[5])'}
    $functions=[regex]::Matches($source,'static void '+$name+'\(')
    if($functions.Count -ne 2){throw 'Expected two arithmetic graphs'}
    $body=Body $source ($source.IndexOf('{',$functions[1].Index))
    $constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\('); if(!$loop.Success){throw 'Missing codelet loop'}
    $operations=Body $body ($body.IndexOf('{',$loop.Index))
    $operations=[regex]::Replace($operations,'WS\((?:rs|csr|csi|is|os),\s*(\d+)\)','$1')
    if($kind -eq 'real') {
        $operations=[regex]::Replace($operations,'R([01])\[(\d+)\]',{param($m) 'samples['+(2*[int]$m.Groups[2].Value+[int]$m.Groups[1].Value)+']'})
    }
    $operations=[regex]::Replace($operations,'\bE\b','precise float')
    if($operations -match '\b(?:WS|R0|R1|stride|MAKE_VOLATILE_STRIDE)\b'){throw 'Untranslated scalar operation'}
    $shader+="`n$signature {`n"
    foreach($constant in $constants){$shader+="const float $($constant.Groups[1].Value)=$($constant.Groups[2].Value);`n"}
    if($kind -eq 'real'){$shader+="for(int k=0;k<5;++k){Cr[k]=0.0;Ci[k]=0.0;}`n"}
    $shader+=$operations+"`n}`n"
}
$shader+=@'
void main(){
    if(base_fft_abi!=81u)return;
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;
    precise vec2 spectrum[64];
    for(uint x=0u;x<8u;++x){
        float samples[8],r[5],im[5];
        for(uint y=0u;y<8u;++y)samples[y]=src.value[batch*128u+y*8u+x].x;
        real8(samples,r,im);
        for(uint ky=0u;ky<5u;++ky)spectrum[ky*8u+x]=vec2(r[ky],im[ky]);
    }
    for(uint ky=0u;ky<5u;++ky){
        float r[8],im[8],ro[8],io[8];
        for(uint x=0u;x<8u;++x){r[x]=spectrum[ky*8u+x].x;im[x]=spectrum[ky*8u+x].y;}
        complex8(r,im,ro,io);
        for(uint kx=0u;kx<8u;++kx)spectrum[ky*8u+kx]=vec2(ro[kx],io[kx]);
    }
    for(uint ky=5u;ky<8u;++ky)for(uint kx=0u;kx<8u;++kx){
        vec2 value=spectrum[(8u-ky)*8u+(8u-kx)%8u];
        spectrum[ky*8u+kx]=vec2(value.x,-value.y);
    }
    for(uint i=0u;i<128u;++i)dst.value[batch*128u+i]=i<64u?spectrum[i]:vec2(0.0);
}
'@
$utf8=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'base_fft.comp'),$shader,$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'r2cf_8.c'),$realSource,$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'n1_8.c'),$complexSource,$utf8)
Copy-Item (Join-Path $PSScriptRoot 'fftw_reference/COPYING') (Join-Path $OutputDirectory 'COPYING')
