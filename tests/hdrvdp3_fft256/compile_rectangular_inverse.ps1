param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin',[switch]$Forward,[switch]$Base,[switch]$Combined,[switch]$Selection,[switch]$MapABI)
$ErrorActionPreference='Stop'
if(([int][bool]$Forward+[int][bool]$Base+[int][bool]$Combined+[int][bool]$Selection) -gt 1){throw 'Choose one rectangular shader mode'}
$stem=if($Selection){'rectangular_residual'}elseif($Combined){'rectangular_reference'}elseif($Base){'rectangular_base'}elseif($Forward){'rectangular_forward'}else{'rectangular_inverse'}
if($MapABI -and !$Selection){throw 'MapABI requires Selection'}
if($MapABI){& "$PSScriptRoot/../generate_${stem}_shader.ps1" -OutputDirectory $OutputDirectory -MapABI}
else{& "$PSScriptRoot/../generate_${stem}_shader.ps1" -OutputDirectory $OutputDirectory}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path
& "$VulkanBin/glslc.exe" "$root/$stem.comp" -o "$root/default.spv" --target-env=vulkan1.2
if($LASTEXITCODE){throw 'Compile failed'}
& "$VulkanBin/spirv-dis.exe" "$root/default.spv" -o "$root/default.spvasm"
if($LASTEXITCODE){throw 'Disassembly failed'}
$asm=[IO.File]::ReadAllText("$root/default.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 32 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 32 1 1',"OpExecutionMode %main LocalSize 32 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$root/$stem.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$root/$stem.spvasm" -o "$root/$stem.spv"
if($LASTEXITCODE){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$root/$stem.spv"
if($LASTEXITCODE){throw 'SPIR-V validation failed'}
Get-FileHash "$root/$stem.comp","$root/$stem.spv"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
