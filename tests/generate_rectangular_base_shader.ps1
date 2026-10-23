# FFTW GPL-2.0-or-later residual8x16/16x8 arithmetic;isolated ABI83.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_base16_fft_shader.ps1" -OutputDirectory "$OutputDirectory/base16"
& "$PSScriptRoot/generate_base_fft_shader.ps1" -OutputDirectory "$OutputDirectory/base8"
$large=[IO.File]::ReadAllText("$OutputDirectory/base16/base16.comp")
$small=[IO.File]::ReadAllText("$OutputDirectory/base8/base_fft.comp")
$shader=$large.Substring(0,$large.IndexOf('void main(){')).Replace('Output {float value[];}','Output {vec2 value[];}')
$start=$small.IndexOf('void real8(');$end=$small.IndexOf('void main(){')
if($start -lt 0 -or $end -lt $start){throw 'Unexpected base8 source'}
$shader+=$small.Substring($start,$end-$start)
$shader+=@'
layout(constant_id=31) const uint rectangular_base_abi=83u;
void main(){
    if(rectangular_base_abi!=83u||gl_GlobalInvocationID.x!=0u)return;
    uint h=p.n,w=p.batches;
    if(!((h==8u&&w==16u)||(h==16u&&w==8u)))return;
    precise vec2 spectrum[128];uint compact=h/2u+1u;
    for(uint x=0u;x<w;++x){
        if(h==8u){
            float samples[8],r[5],im[5];for(uint y=0u;y<8u;++y)samples[y]=src.value[x*h+y].x;
            real8(samples,r,im);for(uint y=0u;y<5u;++y)spectrum[y*w+x]=vec2(r[y],im[y]);
        }else{
            float samples[16],r[9],im[9];for(uint y=0u;y<16u;++y)samples[y]=src.value[x*h+y].x;
            real16(samples,r,im);for(uint y=0u;y<9u;++y)spectrum[y*w+x]=vec2(r[y],im[y]);
        }
    }
    for(uint y=0u;y<compact;++y){
        if(w==8u){
            float r[8],im[8],ro[8],io[8];for(uint x=0u;x<8u;++x){r[x]=spectrum[y*w+x].x;im[x]=spectrum[y*w+x].y;}
            complex8(r,im,ro,io);for(uint x=0u;x<8u;++x)spectrum[y*w+x]=vec2(ro[x],io[x]);
        }else{
            float r[16],im[16],ro[16],io[16];for(uint x=0u;x<16u;++x){r[x]=spectrum[y*w+x].x;im[x]=spectrum[y*w+x].y;}
            complex16(r,im,ro,io);for(uint x=0u;x<16u;++x)spectrum[y*w+x]=vec2(ro[x],io[x]);
        }
    }
    for(uint y=compact;y<h;++y)for(uint x=0u;x<w;++x){vec2 z=spectrum[(h-y)*w+(w-x)%w];spectrum[y*w+x]=vec2(z.x,-z.y);}
    for(uint y=0u;y<h;++y)for(uint x=0u;x<w;++x)dst.value[x*h+y]=spectrum[y*w+x];
}
'@
[IO.File]::WriteAllText("$OutputDirectory/rectangular_base.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
