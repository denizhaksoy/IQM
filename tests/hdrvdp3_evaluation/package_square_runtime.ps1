param([Parameter(Mandatory=$true)][string]$AssetsDirectory,[Parameter(Mandatory=$true)][string]$ModelDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh square package required'}
$items=[ordered]@{}
foreach($name in @('forward','inverse','io','map','spyr_fma','spyr_separate','mean','difference','residual','pool_fma','pool_separate','comparison','product','base')){$items[$name]="$AssetsDirectory/$name.spv"}
$items.lo0="$ModelDirectory/lo0.txt";$items.lo="$ModelDirectory/lo.txt";$items.high="$ModelDirectory/hi0.txt";$items.orientation="$ModelDirectory/orient.txt"
foreach($file in $items.Values){if(!(Test-Path $file -PathType Leaf)){throw "Missing square asset:$file"}}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName;New-Item -ItemType Directory "$out/assets","$out/licenses"|Out-Null
$lines=@('IQM_HDRVDP3_PATHWAY 2 luminance-30ppd-age24-D65-sp0-map13-square96');$records=@()
foreach($key in $items.Keys){
    $ext=if($key-in @('lo0','lo','high','orientation')){'.txt'}else{'.spv'};$relative="assets/$key$ext"
    Copy-Item -LiteralPath $items[$key] -Destination "$out/$relative"
    $hash=(Get-FileHash "$out/$relative").Hash.ToLowerInvariant()
    $lines+=$key+' "'+$relative+'" '+$hash
    $records+=[pscustomobject]@{key=$key;file=$relative;sha256=$hash}
}
[IO.File]::WriteAllLines("$out/pathway.runtime",$lines,[Text.UTF8Encoding]::new($false))
$records|ConvertTo-Json|Set-Content "$out/payloads.json"
Copy-Item "$PSScriptRoot/../fftw_reference/COPYING" "$out/licenses/FFTW-COPYING"
Copy-Item "$PSScriptRoot/../math_reference/amd_win_libm/LICENSE.txt" "$out/licenses/AMD-LICENSE.txt"
Copy-Item "$PSScriptRoot/../SQUARE_PUBLIC_RUNTIME_VALIDATION.md" "$out/VALIDATION.md"
Copy-Item "$PSScriptRoot/../SQUARE_QUALITY_VALIDATION.md" "$out/SQUARE_QUALITY_VALIDATION.md"
Copy-Item "$PSScriptRoot/../GENERAL_BASE_MEAN_VALIDATION.md" "$out/GENERAL_BASE_MEAN_VALIDATION.md"
Copy-Item "$PSScriptRoot/../GENERAL_BASE_PIPELINE_VALIDATION.md" "$out/GENERAL_BASE_PIPELINE_VALIDATION.md"
