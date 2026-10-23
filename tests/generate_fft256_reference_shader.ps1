# Combines validated FFT256 graphs under LocalAdaptFFT's8/9 forward,3/4 inverse ABI.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft256_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/inverse"
& "$PSScriptRoot/generate_fft256_forward_shader.ps1" -OutputDirectory "$OutputDirectory/forward"
$inverse=[IO.File]::ReadAllText("$OutputDirectory/inverse/inverse256.comp")
$forward=[IO.File]::ReadAllText("$OutputDirectory/forward/forward256.comp")
$imain=$inverse.IndexOf('void main(){');$fmain=$forward.IndexOf('void main(){');$helpers=$forward.IndexOf('void r2cf_8')
if($imain -lt 0 -or $fmain -lt 0 -or $helpers -lt 0 -or $helpers -ge $fmain){throw 'Unexpected component source'}
$shader=$inverse.Substring(0,$imain)+$forward.Substring($helpers,$fmain-$helpers)
$shader+=$inverse.Substring($imain).Replace('void main(){','void inverseAxis(){')
$shader+=$forward.Substring($fmain).Replace('void main(){','void forwardAxis(){').Replace('p.graph==3u','p.graph==8u')
$shader+=@'
layout(constant_id=31) const uint reference_fft_abi=256u;
void main(){
    if(reference_fft_abi!=256u||p.n!=256u)return;
    if(p.graph==8u||p.graph==9u)forwardAxis();
    else if(p.graph==3u||p.graph==4u)inverseAxis();
}
'@
[IO.File]::WriteAllText("$OutputDirectory/reference256.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
