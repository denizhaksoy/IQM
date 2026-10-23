# Explicit ABI15 residual extensions;ABI13/14 generation remains unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_map128_shader.ps1" -OutputDirectory "$OutputDirectory/profile14" -VulkanBin $VulkanBin
$map=[IO.File]::ReadAllText("$OutputDirectory/profile14/map128.comp")
function Replace-Exact([string]$from,[string]$to,[int]$count=1){
 if([regex]::Matches($script:map,[regex]::Escape($from)).Count -ne $count){throw "Unexpected source:$from"}
 $script:map=$script:map.Replace($from,$to)
}
Replace-Exact 'map_abi=14u' 'map_abi=15u'
Replace-Exact 'map_abi!=14u' 'map_abi!=15u'
Replace-Exact 'pc.width==256u?1.0/256.0:1.0/64.0' 'pc.width==256u?1.0/256.0:(pc.width==128u?1.0/128.0:1.0/64.0)' 2
Replace-Exact 'for(uint x=0u;x<pc.width;++x) for(uint y=0u;y<pc.width;++y)' 'for(uint x=0u;x<pc.width;++x) for(uint y=0u;y<pc.height;++y)'
Replace-Exact 'selected=x*pc.width+y;' 'selected=x*pc.height+y;'
Replace-Exact 'output_data.words[pc.width*pc.width]=selected;' 'output_data.words[pc.width*pc.height]=selected;'
[IO.File]::WriteAllText("$OutputDirectory/map15.comp",$map,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/glslc.exe" "$OutputDirectory/map15.comp" --target-env=vulkan1.2 -o "$OutputDirectory/default.spv"
if($LASTEXITCODE){throw 'Map15 compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE){throw 'Map15 disassembly failed'}
$asm=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$OutputDirectory/map15.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/map15.spvasm" -o "$OutputDirectory/map15.spv"
if($LASTEXITCODE){throw 'Map15 assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/map15.spv"
if($LASTEXITCODE){throw 'Map15 validation failed'}
Get-FileHash "$OutputDirectory/map15.comp","$OutputDirectory/map15.spv"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/hashes.json"
