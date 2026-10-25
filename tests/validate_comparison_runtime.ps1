param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop';$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path;$records=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $manifest="validation/actual_content_stress_639284716222632999/$pattern/assets_cpu_filters.json"
 $a=Get-Content $manifest -Raw|ConvertFrom-Json
 if(!$ResultsOnly){$ErrorActionPreference='Continue';& $exe $a.comparisonShader $a.comparisonNative "$root/$pattern" *> "$root/$pattern.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "$pattern failed:$code"}}
 if(!(Test-Path "$root/$pattern/PASS.txt")){throw 'Missing PASS'}
 if((Get-Content "$root/$pattern.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan errors'}
 $rows=@(Import-Csv "$root/$pattern/results.tsv" -Delimiter "`t");if($rows.Count -ne 90){throw 'Incomplete cases'}
 $binary=0;$doubles=0;foreach($r in $rows){if([int]$r.binary_errors -ne 0 -or [int]$r.double_bit_errors -ne 0){throw 'Parity mismatch'};$binary+=[int]$r.binary_words;$doubles+=[int]$r.double_values}
 if($binary -ne 691200 -or $doubles -ne 321840){throw 'Incomplete values'}
 $records+=[pscustomobject]@{pattern=$pattern;cases=90;binary_words=$binary;double_words=$doubles;bit_errors=0}
 $hashes+=Get-FileHash -LiteralPath $manifest,$a.comparisonShader|Select-Object Path,Hash
 Write-Output "$pattern runtime comparison PASS"
}
$records|ConvertTo-Json|Set-Content "$root/runtime_results.json"
$hashes+=Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash
$hashes|Sort-Object Path -Unique|ConvertTo-Json|Set-Content "$root/audit_hashes.json"
Set-Content "$root/PASS.txt" 'Runtime comparison:270 cases,2073600 binary words and965520 double words bitwise exact;no Vulkan errors. Overlapping resize cases,not270 unique image pairs.'
