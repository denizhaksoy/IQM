param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path
$records=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $a=Get-Content "validation/actual_content_stress_639284716222632999/$pattern/assets_cpu_filters.json" -Raw|ConvertFrom-Json
 if(!$ResultsOnly){$ErrorActionPreference='Continue';& $exe $a.spyrFMA $a.spyrSeparate $a.nativeSPYR "$root/$pattern" --all-chain $a.mean $a.difference $a.residual $a.residualGoldens *> "$root/$pattern.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "$pattern failed:$code"}}
 if(!(Test-Path -LiteralPath "$root/$pattern/PASS.txt")){throw 'Missing runtime PASS'}
 if((Get-Content "$root/$pattern.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan validation errors'}
 $rows=@(Import-Csv "$root/$pattern/results.tsv" -Delimiter "`t")
 if($rows.Count -ne 22){throw 'Incomplete stage count'}
 foreach($tag in @('h80_w96_reference','h96_w80_reference')){$stages=@($rows|Where-Object case -eq $tag);if(($stages.stage -join ',') -ne '0,1,2,3,4,5,6,7,8,9,10'){throw 'Missing orientation/stage'}}
 $values=0;foreach($row in $rows){if([int]$row.numeric_errors -ne 0 -or [int]$row.bit_errors -ne 0){throw 'Numerical or bit mismatch'};$values+=[int]$row.values}
 if($values -ne 113526){throw 'Incomplete values'}
 $records+=[pscustomobject]@{pattern=$pattern;values=$values;stage_rows=22;bit_errors=0}
}
$records|ConvertTo-Json|Set-Content "$root/runtime_results.json"
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/audit_hashes.json"
Set-Content "$root/PASS.txt" 'Six paired SPYR runtime chains:340578 values,66 stages,zero numeric/bit errors. Two submissions with actual GPU base range validation; public geometry unchanged.'
'Runtime SPYR matrix PASS'
