param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='')
$ErrorActionPreference='Stop'

if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
$tests=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory $OutputDirectory|Out-Null
Copy-Item "$tests/mean_general_difference_pack.comp" "$OutputDirectory/mean_general_difference_pack.comp"
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$output/mean_general_difference_pack.comp" -o "$output/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE -ne 0){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$output/default_controls.spv" -o "$output/default_controls.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$source=[IO.File]::ReadAllText("$output/default_controls.spvasm")
if($source -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected SPIR-V entrypoint'}
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$output/mean_general_difference_pack.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$output/mean_general_difference_pack.spvasm" -o "$output/mean_general_difference_pack.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/mean_general_difference_pack.spv"
if($LASTEXITCODE -ne 0){throw 'SPIR-V validation failed'}
Get-FileHash "$output/mean_general_difference_pack.spv","$output/mean_general_difference_pack.comp"|ConvertTo-Json|Set-Content "$output/hashes.json"
