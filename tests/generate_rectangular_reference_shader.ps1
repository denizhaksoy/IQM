# Combined rectangular FFT ABI64256;separate square assets remain unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_rectangular_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/inverse"
& "$PSScriptRoot/generate_rectangular_forward_shader.ps1" -OutputDirectory "$OutputDirectory/forward"
$inverse=[IO.File]::ReadAllText("$OutputDirectory/inverse/rectangular_inverse.comp")
$forward=[IO.File]::ReadAllText("$OutputDirectory/forward/rectangular_forward.comp")
$cut=$inverse.IndexOf('layout(constant_id=31) const uint rectangular_inverse_abi=')
$start=$forward.IndexOf('void r2cf_8');$end=$forward.IndexOf('layout(constant_id=31) const uint rectangular_forward_abi=')
$store=$forward.IndexOf('void storeRectangle(')
if($cut -lt 0 -or $start -lt 0 -or $end -lt $start -or $store -lt $end){throw 'Unexpected component layout'}
$shader=$inverse.Substring(0,$cut)+$forward.Substring($start,$end-$start)
$imain=$inverse.IndexOf('void main(){',$cut)
$shader+=$inverse.Substring($imain).Replace('void main(){','void inverseAxes(){').Replace('if(rectangular_inverse_abi!=128256u)return;','')
$shader+=$forward.Substring($store).Replace('void main(){','void forwardAxes(){').Replace('if(rectangular_forward_abi!=256128u)return;','')
$shader+=@'
layout(constant_id=31) const uint reference_rectangular_abi=64256u;
void main(){
    if(reference_rectangular_abi!=64256u)return;
    if(p.graph==8u||p.graph==9u)forwardAxes();
    else if(p.graph==3u||p.graph==4u)inverseAxes();
}
'@
[IO.File]::WriteAllText("$OutputDirectory/rectangular_reference.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
