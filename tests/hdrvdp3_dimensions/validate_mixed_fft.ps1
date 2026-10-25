param([Parameter(Mandatory=$true)][string]$NativeDirectory,
 [Parameter(Mandatory=$true)][string]$PlanExecutable,
 [Parameter(Mandatory=$true)][string]$NormalizationExecutable,
 [Parameter(Mandatory=$true)][string]$NormalizationShader,
 [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$native=(Resolve-Path -LiteralPath $NativeDirectory).Path
$plan=(Resolve-Path -LiteralPath $PlanExecutable).Path
$normalizer=(Resolve-Path -LiteralPath $NormalizationExecutable).Path
$shader=(Resolve-Path -LiteralPath $NormalizationShader).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
$out=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$lines=@();$summary=@()
foreach($shape in @(@(80,96),@(96,80))){foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $name="h$($shape[0])_w$($shape[1])_$pattern";$fixture="$native/native_$name";$folder="$out/plans_$name"
 $ErrorActionPreference='Continue';& $plan --mixed-plan $fixture $folder (2*$shape[0]) (2*$shape[1]) *> "$out/$name.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 if($code){throw "Plan inspection failed:$name"}
 $records=Import-Csv "$folder/results.tsv" -Delimiter "`t"
 $matched=@($records|Where-Object {$_.forward_mismatches -eq '0' -and $_.inverse_div_mismatches -eq '0'})
 if($records.Count -ne 32 -or $matched.Count -ne 8){throw "Plan arithmetic domain changed:$name"}
 foreach($label in @('ref','test')){foreach($chain in @('mtf','la')){
  $raw="$folder/${label}_${chain}_inverse_unscaled.txt";$middle=if($chain -eq 'mtf'){'_'}else{'_la_'}
  $expected="$fixture/${label}${middle}native_inverse.txt"
  if(!(Test-Path -LiteralPath $raw)){throw "Missing selected exact stream:$name"}
  $lines+=('{0} "{1}" "{2}"' -f "${name}_${label}_${chain}",$raw.Replace('\','/'),$expected.Replace('\','/'))
 }}
 $summary+=[pscustomobject]@{case=$name;candidates=32;exact_candidates=8;selected='3threads,unaligned,outplace,fresh estimate';normalization='Float32 division once by30720'}
 $summary|ConvertTo-Json|Set-Content "$out/plan_summary.json"
}}
$lines|Set-Content "$out/normalization_manifest.txt"
$ErrorActionPreference='Continue';& $normalizer $shader "$out/normalization_manifest.txt" "$out/gpu_normalization" *> "$out/gpu_normalization.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop'
Get-Content "$out/gpu_normalization.log" -Tail 3
if($code -or (Select-String -LiteralPath "$out/gpu_normalization.log" -Pattern 'VUID-|Validation Error|\[ERROR\]')){throw 'Exact normalization failed'}
Get-FileHash -LiteralPath $plan,$normalizer,$shader,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/hashes.json"
Set-Content "$out/PASS.txt" '192native plan candidates;24 exact streams;fixed30720 GPU division1054696synthetic+1474560native words bitwise exact;notGPUFFT.'
