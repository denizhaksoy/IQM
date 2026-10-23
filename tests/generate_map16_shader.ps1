# Explicit rectangular maps ABI16;preserves priorABI13/14/15 assets.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_map_rectangular_shader.ps1" -OutputDirectory "$OutputDirectory/profile15" -VulkanBin $VulkanBin
$map=[IO.File]::ReadAllText("$OutputDirectory/profile15/map15.comp")
function Replace-Exact([string]$from,[string]$to,[int]$count=1){
 if([regex]::Matches($script:map,[regex]::Escape($from)).Count -ne $count){throw "Unexpected source:$from"}
 $script:map=$script:map.Replace($from,$to)
}
Replace-Exact 'map_abi=15u' 'map_abi=16u'
Replace-Exact 'map_abi!=15u' 'map_abi!=16u'
Replace-Exact 'for(int dx=-1;dx<=1;++dx) for(int dy=1;dy>=-1;--dy) {' @'
for(int outer=-1;outer<=1;++outer) for(int inner=1;inner>=-1;--inner) {
            int dx=pc.width>pc.height?inner:outer,dy=pc.width>pc.height?outer:inner;
'@
Replace-Exact 'if(pc.height>=32u) sum=positive_fma' 'if(max(pc.width,pc.height)>=32u) sum=positive_fma'
[IO.File]::WriteAllText("$OutputDirectory/map16.comp",$map,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/glslc.exe" "$OutputDirectory/map16.comp" --target-env=vulkan1.2 -o "$OutputDirectory/default.spv"
if($LASTEXITCODE){throw 'Map16 compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE){throw 'Map16 disassembly failed'}
$asm=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if($asm -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected entrypoint'}
$asm=$asm.Replace('OpCapability Shader',"OpCapability Shader`n OpCapability DenormPreserve`n OpCapability RoundingModeRTE")
$asm=$asm.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n OpExecutionMode %main DenormPreserve 32`n OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$OutputDirectory/map16.spvasm",$asm)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/map16.spvasm" -o "$OutputDirectory/map16.spv"
if($LASTEXITCODE){throw 'Map16 assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/map16.spv"
if($LASTEXITCODE){throw 'Map16 validation failed'}
Get-FileHash "$OutputDirectory/map16.comp","$OutputDirectory/map16.spv"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/hashes.json"
