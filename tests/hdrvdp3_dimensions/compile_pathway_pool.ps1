param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='',[switch]$NonFma)
$ErrorActionPreference='Stop'

if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
$tests=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
Copy-Item -LiteralPath "$tests/pathway_pool.comp" -Destination $OutputDirectory
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
$reference=Join-Path (Split-Path $tests -Parent) 'shaders/hdrvdp3/hdrvdp3_map_reference.glsl'
$map=[IO.File]::ReadAllText($reference)
$begin=$map.IndexOf('// Unsigned 64-bit helpers;');$end=$map.IndexOf('// Finite IEEE division')
if($begin -lt 0 -or $end -le $begin){throw 'Missing accepted positive FMA helpers'}
$pool=[IO.File]::ReadAllText("$output/pathway_pool.comp")
if([regex]::Matches($pool,'void main\(\)\{').Count -ne 1){throw 'Unexpected entrypoint'}
$pool=$pool.Replace('void main(){',$map.Substring($begin,$end-$begin)+"`nvoid main(){")
[IO.File]::WriteAllText("$output/pathway_pool.comp",$pool)
Get-FileHash -LiteralPath $reference | ConvertTo-Json | Set-Content "$output/helper_source_hash.json"
$defines=@();if($NonFma){$defines=@('-DIQM_CORRDN_NON_FMA')}
& "$VulkanBin/glslc.exe" @defines "$output/pathway_pool.comp" -o "$output/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE -ne 0){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$output/default_controls.spv" -o "$output/default_controls.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$source=[IO.File]::ReadAllText("$output/default_controls.spvasm")
if($source -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected SPIR-V entrypoint'}
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$output/pathway_pool.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$output/pathway_pool.spvasm" -o "$output/pathway_pool.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/pathway_pool.spv"
if($LASTEXITCODE -ne 0){throw 'SPIR-V validation failed'}
Get-FileHash "$output/pathway_pool.spv","$output/pathway_pool.comp"|ConvertTo-Json|Set-Content "$output/hashes.json"
