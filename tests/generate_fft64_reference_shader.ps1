# Opt-in scalar FFTW3.3.10 graph translation; GPL-2.0-or-later.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$archive=Join-Path $root 'validation/fftw-3.3.10-inspection.tar.gz'
if((Get-FileHash $archive).Hash -ne '56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467'){throw 'FFTW archive hash mismatch'}
if(Test-Path $OutputDirectory){throw 'Use a fresh output directory'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
function Body([string]$source,[int]$opening){
    $depth=0
    for($i=$opening;$i -lt $source.Length;++$i){
        if($source[$i] -eq '{'){++$depth}
        if($source[$i] -eq '}'){--$depth;if($depth -eq 0){return $source.Substring($opening+1,$i-$opening-1)}}
    }
    throw 'Unbalanced FFTW braces'
}
$shader=@'
#version 450
// SPDX-License-Identifier: GPL-2.0-or-later
// FFTW3.3.10 scalar r2cf_64/n1_64 graphs;not a production FFT backend.
// Copyright(c)2003,2007-14 Matteo Frigo and Massachusetts Institute of Technology.
// Modified2026 for isolated IQM numerical tests;COPYING and sources retained.
layout(local_size_x=32) in;
layout(constant_id=31) const uint fft64_abi=6401u;
layout(std430,binding=0) readonly buffer Input{vec2 value[];}src;
layout(std430,binding=1) writeonly buffer Output{vec2 value[];}dst;
layout(push_constant) uniform Parameters{uint n,batches,graph,fused;}p;
float FMA(float a,float b,float c){precise float product=a*b;return product+c;}
float FNMS(float a,float b,float c){precise float product=a*b;return c-product;}
float FMS(float a,float b,float c){precise float product=a*b;return product-c;}
float FNMA(float a,float b,float c){precise float product=a*b;return -(product+c);}
'@
foreach($kind in @('real','complex')){
    $name='n1_64';$path='dft/scalar/codelets/n1_64.c';$signature='void complex64(in float ri[64],in float ii[64],out float ro[64],out float io[64])'
    if($kind -eq 'real'){$name='r2cf_64';$path='rdft/scalar/r2cf/r2cf_64.c';$signature='void real64(in float samples[64],out float Cr[33],out float Ci[33])'}
    $source=(& tar -xOf $archive "fftw-3.3.10/$path") -join "`n"
    if($LASTEXITCODE -ne 0){throw 'FFTW extraction failed'}
    [IO.File]::WriteAllText((Join-Path $OutputDirectory "$name.c"),$source,[Text.UTF8Encoding]::new($false))
    $functions=[regex]::Matches($source,'static void '+$name+'\(')
    if($functions.Count -ne 2){throw 'Expected two scalar graphs'}
    $body=Body $source ($source.IndexOf('{',$functions[1].Index))
    $constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\(');if(!$loop.Success){throw 'Missing codelet loop'}
    $operations=Body $body ($body.IndexOf('{',$loop.Index))
    $operations=[regex]::Replace($operations,'WS\((?:rs|csr|csi|is|os),\s*(\d+)\)','$1')
    if($kind -eq 'real'){$operations=[regex]::Replace($operations,'R([01])\[(\d+)\]',{param($m)'samples['+(2*[int]$m.Groups[2].Value+[int]$m.Groups[1].Value)+']'})}
    $operations=[regex]::Replace($operations,'\bE\b','precise float')
    if($operations -match '\b(?:WS|R0|R1|stride|MAKE_VOLATILE_STRIDE)\b'){throw 'Untranslated FFTW operation'}
    $shader+="`n$signature {`n"
    foreach($constant in $constants){$shader+="const float $($constant.Groups[1].Value)=$($constant.Groups[2].Value);`n"}
    if($kind -eq 'real'){$shader+="for(int k=0;k<33;++k){Cr[k]=0.0;Ci[k]=0.0;}`n"}
    $shader+=$operations+"`n}`n"
}
$shader+=@'
void main(){
    if(fft64_abi!=6401u||p.n!=64u)return;
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;
    if(p.graph==8u){
        float samples[64],r[33],im[33];
        for(uint y=0u;y<64u;++y)samples[y]=src.value[batch*64u+y].x;
        real64(samples,r,im);
        for(uint ky=0u;ky<33u;++ky)dst.value[batch*64u+ky]=vec2(r[ky],im[ky]);
    }else if(p.graph==9u){
        float r[64],im[64],ro[64],io[64];
        for(uint x=0u;x<64u;++x){vec2 v=src.value[x*64u+batch];r[x]=v.x;im[x]=v.y;}
        complex64(r,im,ro,io);
        for(uint x=0u;x<64u;++x){
            dst.value[x*64u+batch]=vec2(ro[x],io[x]);
            if(batch>0u&&batch<32u)dst.value[((64u-x)%64u)*64u+64u-batch]=vec2(ro[x],-io[x]);
        }
    }else if(p.graph==3u||p.graph==4u){
        float r[64],im[64],ro[64],io[64];
        for(uint k=0u;k<64u;++k){uint index=p.graph==3u?batch*64u+k:k*64u+batch;vec2 v=src.value[index];r[k]=v.x;im[k]=-v.y;}
        complex64(r,im,ro,io);
        for(uint k=0u;k<64u;++k){uint index=p.graph==3u?batch*64u+k:k*64u+batch;precise vec2 v=vec2(ro[k],-io[k]);if(p.graph==4u)v*=1.0/4096.0;dst.value[index]=v;}
    }
}
'@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'fft64.comp'),$shader,[Text.UTF8Encoding]::new($false))
Copy-Item (Join-Path $PSScriptRoot 'fftw_reference/COPYING') (Join-Path $OutputDirectory 'COPYING')
