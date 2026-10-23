# FFTW GPL-2.0-or-later;exact rectangular native3-thread plan composition.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft256_forward_shader.ps1" -OutputDirectory "$OutputDirectory/components"
$source=[IO.File]::ReadAllText("$OutputDirectory/components/forward256.comp")
$main=$source.LastIndexOf('void main(){');if($main -lt 0){throw 'Missing entrypoint'}
$shader=$source.Substring(0,$main).Replace('if(batch>=256u)return;','if(batch>=p.batches)return;')
$shader+=@'
layout(constant_id=31) const uint rectangular_forward_abi=256128u;
void storeRectangle(uint batch,uint frequency,vec2 value){
    uint h=p.batches,w=p.n;
    dst.value[frequency*h+batch]=value;
    if(batch>0u&&batch<h/2u)dst.value[((w-frequency)%w)*h+h-batch]=vec2(value.x,-value.y);
}
void main(){
    if(rectangular_forward_abi!=256128u)return;
    if(!((p.n==128u&&p.batches==256u)||(p.n==256u&&p.batches==128u)))return;
    uint batch=gl_GlobalInvocationID.x;
    if(p.graph==8u){
        if(p.n==256u){firstAxis();return;}
        if(batch>=p.batches)return;
        float samples[128],r[65],im[65];
        for(uint j=0u;j<128u;++j)samples[j]=src.value[batch*128u+j].x;
        real128(samples,r,im);
        for(uint k=0u;k<65u;++k)dst.value[batch*65u+k]=vec2(r[k],im[k]);
        return;
    }
    if(p.graph!=9u)return;
    uint compact=p.batches/2u+1u;if(batch>=compact)return;
    if(p.n==256u){
        vec2 intermediate[256];
        for(uint j=0u;j<16u;++j){
            vec2 x[16],y[16];for(uint l=0u;l<16u;++l)x[l]=src.value[(j+16u*l)*compact+batch];
            forward16(x,y);for(uint k=0u;k<16u;++k)intermediate[j+16u*k]=y[k];
        }
        for(uint k=0u;k<16u;++k){
            vec2 x[16],W[4];for(uint j=0u;j<16u;++j)x[j]=intermediate[j+16u*k];
            for(uint t=0u;t<4u;++t)W[t]=forward256Twiddle[k*4u+t];
            forwardTwiddle16(x,W);for(uint j=0u;j<16u;++j)storeRectangle(batch,k+16u*j,x[j]);
        }
    }else{
        // Three43-vector thread groups:five8-vector direct buffers plus3 tails.
        if(batch%43u<40u){
            vec2 x[128],y[128];for(uint j=0u;j<128u;++j)x[j]=src.value[j*compact+batch];
            forwardTail128(x,y);for(uint j=0u;j<128u;++j)storeRectangle(batch,j,y[j]);
        }else{
            vec2 intermediate[128];
            for(uint j=0u;j<8u;++j){
                vec2 x[16],y[16];for(uint l=0u;l<16u;++l)x[l]=src.value[(j+8u*l)*compact+batch];
                forward16(x,y);for(uint k=0u;k<16u;++k)intermediate[j+8u*k]=y[k];
            }
            for(uint k=0u;k<16u;++k){
                vec2 x[8],W[3];for(uint j=0u;j<8u;++j)x[j]=intermediate[j+8u*k];
                for(uint t=0u;t<3u;++t)W[t]=vec2(twiddles[k*6u+2u*t],twiddles[k*6u+2u*t+1u]);
                forwardTwiddle8(x,W);for(uint j=0u;j<8u;++j)storeRectangle(batch,k+16u*j,x[j]);
            }
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/rectangular_forward.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
