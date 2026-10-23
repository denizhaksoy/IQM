# Isolated128x256/256x128 inverse;derived FFTW graphs retain GPL notices.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft256_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/components"
$source=[IO.File]::ReadAllText("$OutputDirectory/components/inverse256.comp")
$main=$source.IndexOf('void main(){');if($main -lt 0){throw 'Unexpected inverse source'}
$shader=$source.Substring(0,$main)
$shader+=$source.Substring($main).Replace('void main(){','void axis256(){').Replace('*256u+batch]','*p.batches+batch]').Replace('/65536.0','/32768.0')
$shader+=@'
layout(constant_id=31) const uint rectangular_inverse_abi=128256u;
void main(){
    if(rectangular_inverse_abi!=128256u)return;
    if(!((p.n==128u&&p.batches==256u)||(p.n==256u&&p.batches==128u)))return;
    if(p.graph!=3u&&p.graph!=4u)return;
    if(p.n==256u){axis256();return;}
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;
    float realPart[128],imagPart[128];
    if(p.graph==3u){
        for(uint j=0u;j<8u;++j){
            float ri[16],ii[16],ro[16],io[16];
            for(uint l=0u;l<16u;++l){vec2 v=src.value[batch*128u+j+8u*l];ri[l]=v.y;ii[l]=v.x;}
            graph16_1(ri,ii,ro,io);
            for(uint k=0u;k<16u;++k){realPart[j+8u*k]=ro[k];imagPart[j+8u*k]=io[k];}
        }
        for(uint k=0u;k<16u;++k){
            float ri[8],ii[8],W[6];
            for(uint j=0u;j<8u;++j){ri[j]=realPart[j+8u*k];ii[j]=imagPart[j+8u*k];}
            for(uint w=0u;w<6u;++w)W[w]=twiddles[k*6u+w];
            twiddle8_1(ri,ii,W);
            for(uint j=0u;j<8u;++j)dst.value[(k+16u*j)*p.batches+batch]=vec2(ii[j],ri[j]);
        }
    }else{
        for(uint j=0u;j<16u;++j){
            float ri[8],ii[8],ro[8],io[8];
            for(uint l=0u;l<8u;++l){vec2 v=src.value[batch*128u+j+16u*l];ri[l]=v.y;ii[l]=v.x;}
            graph8_1(ri,ii,ro,io);
            for(uint k=0u;k<8u;++k){realPart[j+16u*k]=io[k];imagPart[j+16u*k]=ro[k];}
        }
        for(uint k=0u;k<8u;++k){
            vec2 x[16],W[4];
            for(uint j=0u;j<16u;++j)x[j]=vec2(realPart[j+16u*k],imagPart[j+16u*k]);
            for(uint w=0u;w<4u;++w)W[w]=simdTwiddles[k*4u+w];
            simdTwiddle16(x,W);
            for(uint j=0u;j<16u;++j)dst.value[(k+8u*j)*p.batches+batch]=x[j]/32768.0;
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/rectangular_inverse.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
