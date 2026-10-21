param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$FFTWArchive='',
    [string]$VulkanBin=''
)
$ErrorActionPreference='Stop'
$root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$archive=if($FFTWArchive){(Resolve-Path -LiteralPath $FFTWArchive).Path}else{''}
if($archive -and (Get-FileHash -LiteralPath $archive).Hash -ne '56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467'){throw 'FFTW3.3.10 archive hash mismatch'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
if(Test-Path -LiteralPath $OutputDirectory){throw 'Use a fresh asset directory'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$root/tests/generate_fft_codelet_shader.ps1" -OutputDirectory "$output/fft128"
& "$root/tests/generate_fft64_reference_shader.ps1" -OutputDirectory "$output/fft64" -FFTWArchive $archive
& "$root/tests/generate_base_fft_shader.ps1" -OutputDirectory "$output/base" -FFTWArchive $archive
& "$root/tests/generate_power_shaders.ps1" -OutputDirectory "$output/power"
$compile=@(
    @('fft128/fft128.comp','fft64.spv'),@('fft64/fft64.comp','fft32.spv'),
    @('base/base_fft.comp','base.spv'),@('power/map.comp','map.spv'),
    @('power/source_math/prepare.comp','math/prepare.spv'),
    @('power/source_math/finish.comp','math/finish.spv'),
    @('power/source_math/multiply.comp','math/multiply.spv')
)
New-Item -ItemType Directory -Path "$output/math"|Out-Null
foreach($pair in $compile){
    & "$VulkanBin/glslc.exe" "$output/$($pair[0])" -o "$output/$($pair[1])" --target-env=vulkan1.2
    if($LASTEXITCODE -ne 0){throw "Compilation failed:$($pair[0])"}
    & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/$($pair[1])"
    if($LASTEXITCODE -ne 0){throw "SPIR-V validation failed:$($pair[1])"}
}
$hashes=foreach($pair in $compile){[ordered]@{file=$pair[1];sha256=(Get-FileHash -LiteralPath "$output/$($pair[1])").Hash}}
$expected=Get-Content (Join-Path $PSScriptRoot 'REFERENCE_SHADER_HASHES.json') -Raw|ConvertFrom-Json
foreach($entry in $hashes){
    $pin=@($expected|Where-Object {$_.file -eq $entry.file})
    if($pin.Count -ne 1 -or $pin[0].sha256 -ne $entry.sha256){throw "Generated shader differs from validated binary:$($entry.file)"}
}
$hashes|ConvertTo-Json|Set-Content "$output/shader_hashes.json"
New-Item -ItemType Directory -Path "$output/licenses"|Out-Null
Copy-Item "$root/tests/fftw_reference/COPYING" "$output/licenses/FFTW-COPYING"
Copy-Item "$root/tests/math_reference/amd_win_libm/LICENSE.txt" "$output/licenses/AMD-LICENSE.txt"
$version=(& "$VulkanBin/glslc.exe" --version) -join "`n"
[ordered]@{reference_shaders=7;all_pinned_hashes_identical=$true;fftw_archive_required=$false;fftw_provenance='tests/fftw_reference/runtime/SOURCES.json';amd_provenance='tests/math_reference/SOURCES.json';compiler=$version;target_env='vulkan1.2';exceptional_math_inputs_supported=$false}|ConvertTo-Json|Set-Content "$output/reproduction.json"
Write-Output "Generated7 reference runtime shaders:$output"
