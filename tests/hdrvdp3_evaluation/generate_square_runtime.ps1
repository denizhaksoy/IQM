# Source-only square96 package; old rectangular package remains independent.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin')
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh square source-build directory required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName
function Run([string]$Name,[string]$Script,[string[]]$Extra){
    $ErrorActionPreference='Continue'
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Script @Extra *> "$out/$Name.log"
    $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
    if($code-ne 0){throw "Square source build failed:$Name"}
}
Run rectangular "$PSScriptRoot/generate_pathway_runtime.ps1" @('-OutputDirectory',"$out/common",'-VulkanBin',$VulkanBin)
Run forward "$PSScriptRoot/../hdrvdp3_fft256/compile_fft192square.ps1" @('-OutputDirectory',"$out/forward",'-VulkanBin',$VulkanBin,'-Direction','forward')
Run inverse "$PSScriptRoot/../hdrvdp3_fft256/compile_fft192square.ps1" @('-OutputDirectory',"$out/inverse",'-VulkanBin',$VulkanBin,'-Direction','inverse')
foreach($name in @('square_filter_io','mean144','mean144_difference_pack','residual144_extended','base144')){
    Run $name "$PSScriptRoot/../hdrvdp3_dimensions/compile_$name.ps1" @('-OutputDirectory',"$out/$name",'-VulkanBin',$VulkanBin)
}
New-Item -ItemType Directory "$out/payloads"|Out-Null
$items=[ordered]@{forward="$out/forward/forward192square.spv";inverse="$out/inverse/inverse192square.spv";io="$out/square_filter_io/square_filter_io.spv";mean="$out/mean144/mean144.spv";difference="$out/mean144_difference_pack/mean144_difference_pack.spv";residual="$out/residual144_extended/residual144_extended.spv";base="$out/base144/base144.spv"}
foreach($name in @('map','spyr_fma','spyr_separate','pool_fma','pool_separate','comparison','product')){$items[$name]="$out/common/runtime/assets/$name.spv"}
foreach($name in $items.Keys){Copy-Item -LiteralPath $items[$name] -Destination "$out/payloads/$name.spv"}
Run package "$PSScriptRoot/package_square_runtime.ps1" @('-AssetsDirectory',"$out/payloads",'-ModelDirectory',"$out/common/models",'-OutputDirectory',"$out/runtime")
$pins=Get-Content "$PSScriptRoot/REFERENCE_SQUARE_PATHWAY_HASHES.json" -Raw|ConvertFrom-Json
if($pins.Count-ne 18){throw 'Square pin count'}
foreach($pin in $pins){if((Get-FileHash "$out/runtime/$($pin.file)").Hash-ne $pin.sha256){throw "Square payload differs:$($pin.key)"}}
Set-Content "$out/PASS.txt" '18 source-generated square96 payloads match accepted SHA256;no native installation or prior numerical exports used.'
