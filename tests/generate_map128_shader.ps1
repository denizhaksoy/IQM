# Explicit extended MapGPU ABI14. Default ABI13 generator/assets are unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/source"
$map=[IO.File]::ReadAllText("$OutputDirectory/source/map.comp")
function Replace-Exact([string]$from,[string]$to,[int]$count=1){
    if([regex]::Matches($script:map,[regex]::Escape($from)).Count -ne $count){throw "Unexpected source:$from"}
    $script:map=$script:map.Replace($from,$to)
}
Replace-Exact 'map_abi=13u' 'map_abi=14u'
Replace-Exact 'map_abi!=13u' 'map_abi!=14u'
Replace-Exact 'for(uint k=0u;k<64u;++k)' 'for(uint k=0u;k<pc.width;++k)' 2
Replace-Exact 'exactProduct(sum.x,1.0/64.0)' 'exactProduct(sum.x,pc.width==256u?1.0/256.0:1.0/64.0)' 2
Replace-Exact 'for(uint x=0u;x<8u;++x) for(uint y=0u;y<8u;++y)' 'for(uint x=0u;x<pc.width;++x) for(uint y=0u;y<pc.width;++y)'
Replace-Exact 'uint row=y*8u+x;' 'uint row=y*pc.width+x;'
Replace-Exact 'selected=x*8u+y;' 'selected=x*pc.width+y;'
Replace-Exact 'output_data.words[64]=selected;' 'output_data.words[pc.width*pc.width]=selected;'
Replace-Exact 'uint(top-32+1023)' 'uint(top-40+1023)'
$map=$map.Replace('Exact signed Q32','Exact signed Q40')
[IO.File]::WriteAllText("$OutputDirectory/map128.comp",$map,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/glslc.exe" "$OutputDirectory/map128.comp" --target-env=vulkan1.2 -o "$OutputDirectory/default_controls.spv"
if($LASTEXITCODE){throw 'Shader compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default_controls.spv" -o "$OutputDirectory/default_controls.spvasm"
if($LASTEXITCODE){throw 'Disassembly failed'}
$asm=[IO.File]::ReadAllText("$OutputDirectory/default_controls.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$OutputDirectory/map128.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/map128.spvasm" -o "$OutputDirectory/map128.spv"
if($LASTEXITCODE){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/map128.spv"
if($LASTEXITCODE){throw 'Shader validation failed'}
Get-FileHash "$OutputDirectory/map128.comp","$OutputDirectory/map128.spv"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/hashes.json"
