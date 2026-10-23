# Explicit64x128/128x64 public quality package,30ppd/age24 only.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,
 [Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh asset directory required'}
$out=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$tests=Split-Path $PSScriptRoot -Parent
& "$tests/hdrvdp3_fft256/compile_rectangular_inverse.ps1" -OutputDirectory "$out/fft" -VulkanBin $VulkanBin -Combined
& "$tests/generate_map16_shader.ps1" -OutputDirectory "$out/map" -VulkanBin $VulkanBin
& "$tests/hdrvdp3_fft256/compile_rectangular_inverse.ps1" -OutputDirectory "$out/base" -VulkanBin $VulkanBin -Selection -MapABI
Copy-Item "$out/fft/rectangular_reference.spv" "$out/fft.spv"
Copy-Item "$out/map/map16.spv" "$out/map.spv"
Copy-Item "$out/base/rectangular_residual.spv" "$out/base.spv"
New-Item -ItemType Directory -Path "$out/math","$out/licenses"|Out-Null
foreach($name in @('prepare','finish','multiply')){
 & "$VulkanBin/glslc.exe" "$out/map/profile15/profile14/source/source_math/$name.comp" --target-env=vulkan1.2 -o "$out/math/$name.spv"
 if($LASTEXITCODE){throw "Math compile failed:$name"}
 & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/math/$name.spv"
 if($LASTEXITCODE){throw "Math validation failed:$name"}
}
$pins=Get-Content "$PSScriptRoot/REFERENCE_SHADER_HASHES_RECTANGULAR.json" -Raw|ConvertFrom-Json
$hashes=foreach($pin in $pins){
 $hash=(Get-FileHash -LiteralPath "$out/$($pin.file)").Hash
 if($hash -ne $pin.sha256){throw "Rectangular shader pin changed:$($pin.file)"}
 [ordered]@{file=$pin.file;sha256=$hash}
}
$hashes|ConvertTo-Json|Set-Content "$out/shader_hashes.json"
Copy-Item "$tests/fftw_reference/COPYING" "$out/licenses/FFTW-COPYING"
Copy-Item "$tests/math_reference/amd_win_libm/LICENSE.txt" "$out/licenses/AMD-LICENSE.txt"
[ordered]@{scope='64x128/128x64 positive luminance,30ppd/age24';public_rectangular_API=$true;reference_shaders=6;fft_abi=64256;map_abi=16;base_fft_abi=85;normalization='MTF/LA inverse once1/32768;residual forward unnormalized';mask_weights='Q20/Q40';all_pinned_hashes_identical=$true;target_env='vulkan1.2';compiler=((& "$VulkanBin/glslc.exe" --version)-join "`n")}|ConvertTo-Json|Set-Content "$out/reproduction.json"
Write-Output "Generated6 pinned rectangular quality shaders:$out"
