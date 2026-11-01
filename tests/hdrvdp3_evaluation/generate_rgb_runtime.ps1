param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh RGB source-build required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName
function Run([string]$Name,[string]$Script,[string[]]$Extra){
 $ErrorActionPreference='Continue'
 & powershell -NoProfile -ExecutionPolicy Bypass -File $Script @Extra *> "$out/$Name.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code){throw "RGB generation failed:$Name"}
}
Run base "$PSScriptRoot/generate_square_runtime.ps1" @('-OutputDirectory',"$out/downstream",'-VulkanBin',$VulkanBin)
Run jnd "$PSScriptRoot/../generate_rgb_jnd_shader.ps1" @('-OutputDirectory',"$out/jnd",'-VulkanBin',$VulkanBin)
Run comparison "$PSScriptRoot/../hdrvdp3_dimensions/compile_general_comparison.ps1" @('-OutputDirectory',"$out/comparison",'-VulkanBin',$VulkanBin,'-RGB')
foreach($name in @('projection','adaptation')){
 $ErrorActionPreference='Continue'
 & "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$PSScriptRoot/../../shaders/hdrvdp3/hdrvdp3_rgb_$name.comp" -o "$out/$name.spv" *> "$out/$name.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 if($code){throw "RGB shader compile failed:$name"}
 & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/$name.spv"
 if($LASTEXITCODE){throw "RGB shader validation failed:$name"}
}
Run package "$PSScriptRoot/package_rgb_runtime.ps1" @('-BaseRuntime',"$out/downstream/runtime",'-Projection',"$out/projection.spv",'-Adaptation',"$out/adaptation.spv",'-JND',"$out/jnd/map_rgb.spv",'-Comparison',"$out/comparison/general_comparison.spv",'-OutputDirectory',"$out/runtime")
$pins=Get-Content -LiteralPath "$PSScriptRoot/REFERENCE_RGB_PATHWAY_HASHES.json" -Raw | ConvertFrom-Json
if($pins.Count -ne 4){throw 'RGB pin count'}
foreach($pin in $pins){if((Get-FileHash -LiteralPath "$out/runtime/$($pin.file)").Hash -ne $pin.sha256){throw "RGB payload differs:$($pin.key)"}}
Set-Content "$out/PASS.txt" '22 payloads regenerated from source:18 pinned v2 base +4 pinned RGB shaders. No native installation or previous numerical exports read.'
