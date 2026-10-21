param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin,[switch]$Reductions)
$ErrorActionPreference='Stop'
$stem=if($Reductions){'reductions'}else{'base16'}
$generator=if($Reductions){'generate_base_reductions_shader.ps1'}else{'generate_base16_fft_shader.ps1'}
& "$PSScriptRoot/../$generator" -OutputDirectory $OutputDirectory
$out=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$out/$stem.comp" -o "$out/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$out/default_controls.spv" -o "$out/default_controls.spvasm"
if($LASTEXITCODE){throw 'Disassembly failed'}
$asm=[IO.File]::ReadAllText("$out/default_controls.spvasm")
$local=if($Reductions){64}else{32}
if($asm -notmatch "OpExecutionMode %main LocalSize $local 1 1"){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace("OpExecutionMode %main LocalSize $local 1 1","OpExecutionMode %main LocalSize $local 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$out/$stem.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$out/$stem.spvasm" -o "$out/$stem.spv"
if($LASTEXITCODE){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/$stem.spv"
if($LASTEXITCODE){throw 'SPIR-V validation failed'}
Get-FileHash "$out/$stem.spv","$out/$stem.comp"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/hashes.json"
