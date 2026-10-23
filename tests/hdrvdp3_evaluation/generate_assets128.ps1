# Explicit128/30ppd/age24 runtime asset package. Bounded32/64 assets stay unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
$out=(Resolve-Path -LiteralPath $OutputDirectory).Path;$tests=Split-Path $PSScriptRoot -Parent
& "$tests/hdrvdp3_fft256/compile_inverse.ps1" -OutputDirectory "$out/fft" -VulkanBin $VulkanBin -Combined
& "$tests/generate_map128_shader.ps1" -OutputDirectory "$out/map" -VulkanBin $VulkanBin
& "$tests/hdrvdp3_fft256/compile_base16.ps1" -OutputDirectory "$out/base" -VulkanBin $VulkanBin -MapABI
Copy-Item "$out/fft/reference256.spv" "$out/fft.spv"
Copy-Item "$out/map/map128.spv" "$out/map.spv"
Copy-Item "$out/base/base16.spv" "$out/base.spv"
New-Item -ItemType Directory -Path "$out/math","$out/licenses"|Out-Null
$pins=Get-Content (Join-Path $PSScriptRoot 'REFERENCE_SHADER_HASHES.json') -Raw|ConvertFrom-Json
foreach($name in @('prepare','finish','multiply')){
    & "$VulkanBin/glslc.exe" "$out/map/source/source_math/$name.comp" --target-env=vulkan1.2 -o "$out/math/$name.spv"
    if($LASTEXITCODE){throw "Math compilation failed:$name"}
    & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/math/$name.spv"
    if($LASTEXITCODE){throw "Math SPIR-V validation failed:$name"}
    $expected=@($pins|Where-Object {$_.file -eq "math/$name.spv"})
    if($expected.Count -ne 1 -or (Get-FileHash "$out/math/$name.spv").Hash -ne $expected[0].sha256){throw "Math shader pin changed:$name"}
}
Copy-Item "$tests/fftw_reference/COPYING" "$out/licenses/FFTW-COPYING"
Copy-Item "$tests/math_reference/amd_win_libm/LICENSE.txt" "$out/licenses/AMD-LICENSE.txt"
$files=@('fft.spv','map.spv','base.spv','math/prepare.spv','math/finish.spv','math/multiply.spv')
$hashes=foreach($file in $files){[ordered]@{file=$file;sha256=(Get-FileHash "$out/$file").Hash}}
$hashes|ConvertTo-Json|Set-Content "$out/shader_hashes.json"
$pinPath=Join-Path $PSScriptRoot 'REFERENCE_SHADER_HASHES128.json'
$expected=Get-Content -LiteralPath $pinPath -Raw|ConvertFrom-Json
foreach($entry in $hashes){$pin=@($expected|Where-Object {$_.file -eq $entry.file});if($pin.Count -ne 1 -or $pin[0].sha256 -ne $entry.sha256){throw "128 shader pin changed:$($entry.file)"}}
[ordered]@{scope='128square positive luminance,30ppd,age24;not RGB/general dimensions';reference_shaders=6;fft_abi=256;map_abi=14;base_fft_abi=82;fft_graphs='forward8/9,inverse3/4';normalization='once,inverse1/65536';mask_weights='Q20/Q40';math_pins_unchanged=$true;compiler=((& "$VulkanBin/glslc.exe" --version)-join "`n");target_env='vulkan1.2'}|ConvertTo-Json|Set-Content "$out/reproduction.json"
Write-Output "Generated6 explicit128 runtime shaders:$out"
