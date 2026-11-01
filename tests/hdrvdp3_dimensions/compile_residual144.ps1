param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/../generate_residual144_shader.ps1" -OutputDirectory $OutputDirectory
$out=(Resolve-Path $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$out/residual144.comp" -o "$out/default.spv" --target-env=vulkan1.2
if ($LASTEXITCODE -ne 0) { throw 'GLSL compilation failed' }
& "$VulkanBin/spirv-dis.exe" "$out/default.spv" -o "$out/default.spvasm"
if ($LASTEXITCODE -ne 0) { throw 'SPIR-V disassembly failed' }
$source=[IO.File]::ReadAllText("$out/default.spvasm")
$entry='OpExecutionMode %main LocalSize 64 1 1'
if (!$source.Contains($entry)) { throw 'Unexpected entrypoint' }
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace($entry,"$entry`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$out/residual144.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$out/residual144.spvasm" -o "$out/residual144.spv"
if ($LASTEXITCODE -ne 0) { throw 'SPIR-V assembly failed' }
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/residual144.spv"
if ($LASTEXITCODE -ne 0) { throw 'SPIR-V validation failed' }
Get-FileHash "$out/residual144.comp","$out/residual144.spv" | Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
