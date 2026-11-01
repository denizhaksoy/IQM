param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/base"
$source=[IO.File]::ReadAllText("$OutputDirectory/base/map.comp")
$start=$source.IndexOf('} else if(pc.mode==15u) {');$end=$source.IndexOf('} else if(pc.mode==16u) {',$start)
if($start -lt 0 -or $end -le $start){throw 'JND branch anchor changed'}
$branch=$source.Substring($start,$end-$start)
$before='float before=exactProduct(load_float(i),load_float(parameters+1u+ch));'
if(!$branch.Contains($before)){throw 'LMSR input anchor changed'}
$branch=$branch.Replace('pc.mode==15u','pc.mode==37u').Replace($before,'float before=load_float((ch==2u?3u:ch)*pc.width+i);')
$source=$source.Insert($start,$branch)
$source=$source.Replace('map_abi=13u','map_abi=17u').Replace('map_abi!=13u','map_abi!=17u')
[IO.File]::WriteAllText("$OutputDirectory/map_rgb.comp",$source)
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$OutputDirectory/map_rgb.comp" -o "$OutputDirectory/default.spv"
if($LASTEXITCODE){throw 'Compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE){throw 'Disassemble failed'}
$asm=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$OutputDirectory/map_rgb.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/map_rgb.spvasm" -o "$OutputDirectory/map_rgb.spv"
if($LASTEXITCODE){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/map_rgb.spv"
if($LASTEXITCODE){throw 'Validation failed'}
