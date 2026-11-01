param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin="C:/VulkanSDK/1.4.363.0/Bin",[ValidateSet("forward","inverse")][string]$Direction="forward")
$ErrorActionPreference='Stop'

if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
$tests=Split-Path $PSScriptRoot -Parent
& "$tests/generate_fft192square_shader.ps1" -OutputDirectory $OutputDirectory -Direction $Direction
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$output/${Direction}192square.comp" -o "$output/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE -ne 0){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$output/default_controls.spv" -o "$output/default_controls.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$source=[IO.File]::ReadAllText("$output/default_controls.spvasm")
if($source -notmatch 'OpExecutionMode %main LocalSize 32 1 1'){throw 'Unexpected SPIR-V entrypoint'}
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace('OpExecutionMode %main LocalSize 32 1 1',"OpExecutionMode %main LocalSize 32 1 1`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$output/${Direction}192square.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$output/${Direction}192square.spvasm" -o "$output/${Direction}192square.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/${Direction}192square.spv"
if($LASTEXITCODE -ne 0){throw 'SPIR-V validation failed'}
Get-FileHash "$output/${Direction}192square.spv","$output/${Direction}192square.comp"|ConvertTo-Json|Set-Content "$output/hashes.json"
