# Package already validated shader/model assets; does not compile or overwrite.
param([Parameter(Mandatory=$true)][string]$AssetsManifest,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh package directory required'}
$a=Get-Content -LiteralPath $AssetsManifest -Raw|ConvertFrom-Json
$items=[ordered]@{forward80=$a.forward160;inverse80=$a.inverse160;forward96=$a.forward192;inverse96=$a.inverse192;io=$a.filterIO;spyr_fma=$a.spyrFMA;spyr_separate=$a.spyrSeparate;mean=$a.mean;difference=$a.difference;residual=$a.residual;pool_fma=$a.poolFMA;pool_separate=$a.poolSeparate;comparison=$a.comparisonShader;map=$a.map;product=$a.product;base=$a.base120;lo0="$($a.nativeSPYR)/lo0.txt";lo="$($a.nativeSPYR)/lo.txt";high="$($a.nativeSPYR)/hi0.txt";orientation="$($a.nativeSPYR)/orient.txt"}
foreach($p in $items.Values){if(!(Test-Path -LiteralPath $p -PathType Leaf)){throw "Missing asset:$p"}}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName;New-Item -ItemType Directory "$out/assets","$out/licenses"|Out-Null
$lines=@('IQM_HDRVDP3_PATHWAY 1 luminance-30ppd-age24-D65-sp0-map13');$provenance=@()
foreach($entry in $items.GetEnumerator()){
 $ext=if($entry.Key -in @('lo0','lo','high','orientation')){'.txt'}else{'.spv'};$relative="assets/$($entry.Key)$ext";Copy-Item -LiteralPath $entry.Value -Destination "$out/$relative"
 $hash=(Get-FileHash -LiteralPath "$out/$relative").Hash.ToLowerInvariant();$lines+=$entry.Key+' "'+$relative+'" '+$hash;$provenance+=[pscustomobject]@{key=$entry.Key;source=$entry.Value;file=$relative;sha256=$hash}
}
[IO.File]::WriteAllLines("$out/pathway.runtime",$lines,(New-Object Text.UTF8Encoding($false)))
$provenance|ConvertTo-Json|Set-Content "$out/provenance.json"
Copy-Item tests/fftw_reference/COPYING "$out/licenses/FFTW-COPYING"
Copy-Item tests/math_reference/amd_win_libm/LICENSE.txt "$out/licenses/AMD-LICENSE.txt"
Copy-Item tests/PATHWAY_QUALITY_API_VALIDATION.md "$out/VALIDATION.md"
if(Test-Path "$PSScriptRoot/../PATHWAY_CLI_VALIDATION.md"){Copy-Item "$PSScriptRoot/../PATHWAY_CLI_VALIDATION.md" "$out/CLI.md"}
'Packaged20 runtime assets;fixed30ppd/age24/D65 profile;no compilation.'
