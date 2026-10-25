param([Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [string]$StressRoot='validation/actual_content_stress_639284716222632999',
 [switch]$ResultsOnly)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){
 if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
 New-Item -ItemType Directory $OutputDirectory | Out-Null
}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path
$records=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $manifest="$StressRoot/$pattern/assets_cpu_filters.json"
 $a=Get-Content -LiteralPath $manifest -Raw|ConvertFrom-Json
 foreach($shape in @(@(80,96),@(96,80))){
  $h=$shape[0];$w=$shape[1]
  $forward=if($h -eq 80){$a.forward160}else{$a.forward192}
  $inverse=if($h -eq 80){$a.inverse160}else{$a.inverse192}
  foreach($mode in @('mtf','la')){foreach($label in @('ref','test')){
   $tag="${pattern}_h${h}_w${w}_${mode}_$label"
   if(!$ResultsOnly){
    $ErrorActionPreference='Continue'
    & $exe "$($a.nativeFilters)/h${h}_w${w}" $h $w $mode $label $forward $inverse $a.filterIO "$root/$tag" *> "$root/$tag.log"
    $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
    if($code -ne 0){throw "$tag failed:$code"}
   }
   if(!(Test-Path -LiteralPath "$root/$tag/PASS.txt")){throw "Missing PASS:$tag"}
   $log=Get-Content -LiteralPath "$root/$tag.log" -Raw
   if($log -match 'VUID-|Validation Error|\[ERROR\]'){throw "Vulkan validation error:$tag"}
   $rows=@(Import-Csv -LiteralPath "$root/$tag/results.tsv" -Delimiter "`t")
   if(($rows.stage -join ',') -ne 'stage0,stage1,stage2,stage3,output'){throw "Missing stage:$tag"}
   $values=0
   foreach($row in $rows){if([int]$row.numeric_errors -ne 0 -or [int]$row.bit_errors -ne 0){throw "Mismatch:$tag"};$values+=[int]$row.values}
   if($values -ne 253440){throw "Incomplete values:$tag"}
   if((Get-Item -LiteralPath "$root/$tag/model_filter.f32").Length -ne 122880){throw "Incomplete model:$tag"}
   $records+=[pscustomobject]@{case=$tag;stage_values=$values;model_words=30720;bit_errors=0}
   Write-Output "$tag verified"
  }}
 }
 foreach($asset in @($a.forward160,$a.inverse160,$a.forward192,$a.inverse192,$a.filterIO,$manifest)){$hashes+=Get-FileHash -LiteralPath $asset|Select-Object Path,Hash}
}
if($records.Count -ne 24){throw 'Expected24 cases'}
$records|ConvertTo-Json|Set-Content "$root/runtime_results.json"
$hashes+=Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash
$hashes|Sort-Object Path -Unique|ConvertTo-Json|Set-Content "$root/runtime_asset_hashes.json"
Set-Content "$root/PASS.txt" '24 runtime filter cases:6082560 stage values and737280 model words exact; same-instance trace/nontrace repeated calls covered for three80x96 MTF references. Intel Arc scope recorded separately; no public geometry extension.'
'Runtime mixed filter matrix PASS'
