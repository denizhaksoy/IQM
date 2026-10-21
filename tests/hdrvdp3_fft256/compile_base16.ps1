param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/../generate_base16_fft_shader.ps1" -OutputDirectory $OutputDirectory
$out=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$out/base16.comp" -o "$out/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$out/default_controls.spv" -o "$out/default_controls.spvasm"
if($LASTEXITCODE){throw 'Disassembly failed'}
$asm=[IO.File]::ReadAllText("$out/default_controls.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 32 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 32 1 1',"OpExecutionMode %main LocalSize 32 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$out/base16.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$out/base16.spvasm" -o "$out/base16.spv"
if($LASTEXITCODE){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/base16.spv"
if($LASTEXITCODE){throw 'SPIR-V validation failed'}
Get-FileHash "$out/base16.spv","$out/base16.comp"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/hashes.json"
