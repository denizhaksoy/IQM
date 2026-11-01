# Reuse accepted FFTW scalar graphs; add 10x10 without altering old payloads.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_residual144_extended_shader.ps1" -OutputDirectory "$OutputDirectory/graphs"
$s=[IO.File]::ReadAllText("$OutputDirectory/graphs/residual144_extended.comp")
$start=$s.IndexOf('void main(){');if($start -lt 0){throw 'Missing entrypoint'}
$main=$s.Substring($start)
$guard='if(h!=12u||w!=12u)return;'
if(!$main.Contains($guard)){throw 'Changed dimension anchor'}
$main=$main.Replace($guard,'if(!((h==10u||h==12u)&&(w==10u||w==12u)))return;uint count=h*w;')
$main=$main.Replace('batch*433u','batch*(3u*count+1u)').Replace('288u','(2u*count)').Replace('432u','(3u*count)').Replace('144u','count')
[IO.File]::WriteAllText("$OutputDirectory/residual_general_extended.comp",$s.Substring(0,$start)+$main,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
