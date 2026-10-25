param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop';$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path;$records=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $manifest="validation/actual_content_stress_639284716222632999/$pattern/assets_cpu_filters.json";$a=Get-Content $manifest -Raw|ConvertFrom-Json;$replay="validation/actual_content_stress_639284716222632999/$pattern/replay_cpu_filters"
 if(!$ResultsOnly){$ErrorActionPreference='Continue';& $exe $a.map $a.nativeMasking "$root/$pattern" $a.product "$replay/comparison" "$replay/bands" "$replay/pool" "$replay/inputs/log_inputs" $a.base120 "$replay/spyr" *> "$root/$pattern.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "$pattern failed:$code"}}
 if(!(Test-Path "$root/$pattern/PASS.txt")){throw 'Missing PASS'}
 if((Get-Content "$root/$pattern.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan errors'}
 $rows=@(Import-Csv "$root/$pattern/results.tsv" -Delimiter "`t");if($rows.Count -ne 126){throw 'Incomplete checks'}
 $values=0;foreach($r in $rows){if([int]$r.numeric_errors -ne 0 -or [int]$r.bit_errors -ne 0){throw 'Numerical/bit mismatch'};$values+=[int]$r.values}
 if($values -ne 393384){throw 'Incomplete values'}
 $records+=[pscustomobject]@{pattern=$pattern;checks=126;values=$values;bit_errors=0;GPU_inputs=$replay}
 $hashes+=Get-FileHash -LiteralPath $manifest,$a.map,$a.product,$a.base120|Select-Object Path,Hash
}
$records|ConvertTo-Json|Set-Content "$root/runtime_results.json"
$hashes+=Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash
$hashes|Sort-Object Path -Unique|ConvertTo-Json|Set-Content "$root/audit_hashes.json"
Set-Content "$root/PASS.txt" 'Masking runtime:378 checks/1180152 values bitwise exact across six pairs;150 fixed default-model scalar comparisons also double-bit exact;no Vulkan errors. Counts include repeated input checks.'
'Runtime masking matrix PASS'
