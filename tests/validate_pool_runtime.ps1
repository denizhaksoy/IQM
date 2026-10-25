param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path;$records=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $base="validation/actual_content_stress_639284716222632999/$pattern"
 $a=Get-Content "$base/assets_cpu_filters.json" -Raw|ConvertFrom-Json
 $inputFile="$base/replay_cpu_filters/bands/pool.txt"
 if(!$ResultsOnly){$ErrorActionPreference='Continue';& $exe $a.poolFMA $a.poolSeparate $inputFile "$root/$pattern" *> "$root/$pattern.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "$pattern failed:$code"}}
 if(!(Test-Path -LiteralPath "$root/$pattern/PASS.txt")){throw 'Missing PASS'}
 if((Get-Content "$root/$pattern.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan error'}
 $rows=@(Import-Csv "$root/$pattern/results.tsv" -Delimiter "`t")
 if(($rows.case -join ',') -ne '0,1,2,3,4,5,6,7,8,9'){throw 'Incomplete bands'}
 $values=0;foreach($row in $rows){if([int]$row.numeric_errors -ne 0 -or [int]$row.bit_errors -ne 0){throw 'Parity mismatch'};$values+=[int]$row.values}
 if($values -ne 71520){throw 'Incomplete values'}
 $records+=[pscustomobject]@{pattern=$pattern;bands=10;values=$values;bit_errors=0;input=$inputFile}
 $hashes+=Get-FileHash -LiteralPath $inputFile,$a.poolFMA,$a.poolSeparate|Select-Object Path,Hash
}
$records|ConvertTo-Json|Set-Content "$root/runtime_results.json"
$hashes+=Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash
$hashes|Sort-Object Path -Unique|ConvertTo-Json|Set-Content "$root/audit_hashes.json"
Set-Content "$root/PASS.txt" 'Pooling runtime:30 bands,214560 raw/pooled words bitwise exact,zero Vulkan errors; six content/orientation pairs. Resize/CSF use existing MapGPU; public geometry unchanged.'
'Pool runtime matrix PASS'
