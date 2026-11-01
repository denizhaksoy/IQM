# Isolated next-geometry diagnostic. Reuse the existing FFTW-derived graphs;
# do not modify or recompile the accepted residual120 binaries.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if (Test-Path $OutputDirectory) { throw 'Fresh output required' }
New-Item -ItemType Directory $OutputDirectory | Out-Null
& "$PSScriptRoot/generate_residual120_shader.ps1" -OutputDirectory "$OutputDirectory/graphs"
$source=[IO.File]::ReadAllText("$OutputDirectory/graphs/residual120.comp")
$guard='if(!((h==10u&&w==12u)||(h==12u&&w==10u)))return;'
if (!$source.Contains($guard)) { throw 'Residual graph anchor changed' }
$source=$source.Replace($guard,'if(h!=12u||w!=12u)return;').Replace('batch*120u','batch*144u')
# Octave narrows an entirely real complex result to a real matrix, whose
# exported imaginary components are +0. Preserve all real components and
# retain complex signed-zero components when any imaginary value is nonzero.
# https://docs.octave.org/doxygen/11/de/d87/ov-flt-cx-mat_8cc_source.html
$source=$source.Replace('vec2 halfSpectrum[84];','vec2 halfSpectrum[84];vec2 result[144];')
$source=$source.Replace('dst.value[batch*144u+','result[')
$end=$source.LastIndexOf('}')
$narrow=@'
    bool entirelyReal=true;
    for(uint i=0u;i<144u;++i)if(result[i].y!=0.0)entirelyReal=false;
    for(uint i=0u;i<144u;++i)dst.value[batch*144u+i]=entirelyReal?vec2(result[i].x,0.0):result[i];
'@
$source=$source.Insert($end,$narrow)
[IO.File]::WriteAllText("$OutputDirectory/residual144.comp",$source,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
