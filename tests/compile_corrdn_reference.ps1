param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root 'shaders/hdrvdp3/hdrvdp3_corrdn.glsl'
if(Test-Path -LiteralPath $OutputDirectory) { throw 'Use a fresh output directory; prior binaries must be preserved.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
foreach($variant in @('hdrvdp3_corrdn','hdrvdp3_corrdn_native_tail')) {
    $defines=@(); if($variant.EndsWith('_native_tail')) { $defines=@('-DIQM_CORRDN_NON_FMA') }
    & (Join-Path $VulkanBin 'glslc.exe') @defines $source -o "$output/$variant.spv" --target-env=vulkan1.2
    if($LASTEXITCODE -ne 0) { throw "Shader compilation failed: $variant" }
    & (Join-Path $VulkanBin 'spirv-val.exe') --target-env vulkan1.2 "$output/$variant.spv"
    if($LASTEXITCODE -ne 0) { throw "SPIR-V validation failed: $variant" }
    & (Join-Path $VulkanBin 'glslc.exe') @defines $source -o "$output/$variant.inc" -mfmt=c --target-env=vulkan1.2
    if($LASTEXITCODE -ne 0) { throw "Shader include generation failed: $variant" }
}
$hashes=foreach($file in @($source,"$output/hdrvdp3_corrdn.spv","$output/hdrvdp3_corrdn_native_tail.spv","$output/hdrvdp3_corrdn.inc","$output/hdrvdp3_corrdn_native_tail.inc")) {
    Get-FileHash -LiteralPath $file -Algorithm SHA256 | Select-Object Path,Hash
}
$hashes | ConvertTo-Json | Set-Content "$output/hashes.json"
Write-Output "Validated binaries/includes saved to $output; runtime includes were not replaced."
