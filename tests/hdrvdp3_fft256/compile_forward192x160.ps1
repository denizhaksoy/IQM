param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='',[ValidateSet(0,1)][int]$LeafGraph=0,[ValidateSet(0,1)][int]$TwiddleGraph=1,[ValidateSet(0,1)][int]$ProductsOrder=0,[switch]$PreciseVectorHelpers)
$ErrorActionPreference='Stop'

if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
$tests=Split-Path $PSScriptRoot -Parent
& "$tests/generate_fft192x160_forward_shader.ps1" -OutputDirectory $OutputDirectory -LeafGraph $LeafGraph -TwiddleGraph $TwiddleGraph -ProductsOrder $ProductsOrder -PreciseVectorHelpers:$PreciseVectorHelpers
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$output/forward192x160.comp" -o "$output/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE -ne 0){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$output/default_controls.spv" -o "$output/default_controls.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$source=[IO.File]::ReadAllText("$output/default_controls.spvasm")
if($source -notmatch 'OpExecutionMode %main LocalSize 32 1 1'){throw 'Unexpected SPIR-V entrypoint'}
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace('OpExecutionMode %main LocalSize 32 1 1',"OpExecutionMode %main LocalSize 32 1 1`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$output/forward192x160.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$output/forward192x160.spvasm" -o "$output/forward192x160.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/forward192x160.spv"
if($LASTEXITCODE -ne 0){throw 'SPIR-V validation failed'}
Get-FileHash "$output/forward192x160.spv","$output/forward192x160.comp"|ConvertTo-Json|Set-Content "$output/hashes.json"
