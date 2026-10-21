# Isolated Q20/Q40 integer mask resize, not the accepted MapGPU ABI13 asset.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$VulkanBin)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/source"
$source=[IO.File]::ReadAllText("$OutputDirectory/source/map.comp")
foreach($needle in @('map_abi=13u','map_abi!=13u','uint(top-32+1023)')){
    if([regex]::Matches($source,[regex]::Escape($needle)).Count -ne 1){throw "Unexpected map source:$needle"}
}
$source=$source.Replace('map_abi=13u','map_abi=141u').Replace('map_abi!=13u','map_abi!=141u')
$source=$source.Replace('uint(top-32+1023)','uint(top-40+1023)').Replace('Exact signed Q32','Exact signed Q40')
[IO.File]::WriteAllText("$OutputDirectory/mask_q20.comp",$source,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/glslc.exe" "$OutputDirectory/mask_q20.comp" --target-env=vulkan1.2 -o "$OutputDirectory/mask_q20.spv"
if($LASTEXITCODE){throw 'Shader compile failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/mask_q20.spv"
if($LASTEXITCODE){throw 'Shader validation failed'}
Get-FileHash "$OutputDirectory/mask_q20.comp","$OutputDirectory/mask_q20.spv"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/hashes.json"
