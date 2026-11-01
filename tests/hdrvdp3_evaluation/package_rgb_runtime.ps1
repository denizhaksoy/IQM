param([Parameter(Mandatory=$true)][string]$BaseRuntime,[Parameter(Mandatory=$true)][string]$Projection,[Parameter(Mandatory=$true)][string]$Adaptation,[Parameter(Mandatory=$true)][string]$JND,[Parameter(Mandatory=$true)][string]$Comparison,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh RGB output required'}
$expected='IQM_HDRVDP3_PATHWAY 2 luminance-30ppd-age24-D65-sp0-map13-square96'
if((Get-Content -LiteralPath "$BaseRuntime/pathway.runtime" -First 1) -ne $expected){throw 'RGB requires validated v2 base'}
$items=[ordered]@{projection=$Projection;adaptation=$Adaptation;jnd=$JND;comparison=$Comparison}
foreach($file in $items.Values){if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw "Missing RGB shader:$file"}}
New-Item -ItemType Directory $OutputDirectory | Out-Null
Copy-Item -LiteralPath $BaseRuntime -Destination "$OutputDirectory/base" -Recurse
New-Item -ItemType Directory "$OutputDirectory/assets" | Out-Null
$lines=@('IQM_HDRVDP3_RGB 1 rgb-native-30ppd-age24-sp0-square96')
$hash=(Get-FileHash -LiteralPath "$OutputDirectory/base/pathway.runtime").Hash.ToLowerInvariant()
$lines+='base "base/pathway.runtime" '+$hash
foreach($key in $items.Keys){
 $relative="assets/$key.spv";Copy-Item -LiteralPath $items[$key] -Destination "$OutputDirectory/$relative"
 $hash=(Get-FileHash -LiteralPath "$OutputDirectory/$relative").Hash.ToLowerInvariant();$lines+=$key+' "'+$relative+'" '+$hash
}
[IO.File]::WriteAllLines("$OutputDirectory/rgb.runtime",$lines,[Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath "$PSScriptRoot/../RGB_QUALITY_VALIDATION.md" -Destination "$OutputDirectory/VALIDATION.md"
