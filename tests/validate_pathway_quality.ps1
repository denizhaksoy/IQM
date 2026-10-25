param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop';$exe=(Resolve-Path -LiteralPath $Executable).Path
if(!$ResultsOnly){if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory "$OutputDirectory/configs"|Out-Null}
$root=(Resolve-Path -LiteralPath $OutputDirectory).Path;$records=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $manifest="validation/actual_content_stress_639284716222632999/$pattern/assets_cpu_filters.json";$a=Get-Content $manifest -Raw|ConvertFrom-Json
 foreach($shape in @(@(80,96),@(96,80))){$h=$shape[0];$w=$shape[1];$tag="${pattern}_h${h}_w${w}";$repeat=$pattern -eq 'asymmetric' -and $h -eq 80
  $config="$root/configs/$tag.txt"
  if(!$ResultsOnly){
   $values=[ordered]@{w="$w";h="$h";forward=$(if($h -eq 80){$a.forward160}else{$a.forward192});inverse=$(if($h -eq 80){$a.inverse160}else{$a.inverse192});io=$a.filterIO;spyr_fma=$a.spyrFMA;spyr_separate=$a.spyrSeparate;mean=$a.mean;difference=$a.difference;residual=$a.residual;pool_fma=$a.poolFMA;pool_separate=$a.poolSeparate;comparison=$a.comparisonShader;map=$a.map;product=$a.product;base=$a.base120;native_filters=$a.nativeFilters;native_visual=$a.visual;native_spyr=$a.nativeSPYR;native_masking=$a.nativeMasking;native_residual=$a.residualGoldens;native_comparison=$a.comparisonNative}
   $lines=foreach($v in $values.GetEnumerator()){$v.Key+' "'+([string]$v.Value).Replace('\','/')+'"'};$lines|Set-Content -LiteralPath $config
   $arguments=@($config,"$root/$tag");if($repeat){$arguments+='--repeat'}
   $ErrorActionPreference='Continue';& $exe @arguments *> "$root/$tag.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "$tag failed:$code"}
  }
  if(!(Test-Path "$root/$tag/PASS.txt")){throw 'Missing API PASS'}
  if((Get-Content "$root/$tag.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan errors'}
  $rows=@(Import-Csv "$root/$tag/results.tsv" -Delimiter "`t");$expected=if($repeat){82}else{81};if($rows.Count -ne $expected){throw 'Incomplete stage coverage'}
  foreach($row in $rows){if([int]$row.bit_errors -ne 0){throw 'Stage bit mismatch'}}
  $sum=($rows|ForEach-Object{[int]$_.values}|Measure-Object -Sum).Sum;if($sum -ne $(if($repeat){327737}else{327735})){throw "Incomplete values:$sum"}
  $score=Get-Content "$root/$tag/scores.json" -Raw|ConvertFrom-Json
  $records+=[pscustomobject]@{case=$tag;checks=$expected;values=$sum;Q=$score.Q;Q_JOD=$score.Q_JOD;bit_errors=0;score_only_repeat=$repeat}
  $originals=@("$($a.nativeFilters)/h${h}_w${w}/ref_luminance.txt","$($a.nativeFilters)/h${h}_w${w}/test_luminance.txt")
  $hashes+=Get-FileHash -LiteralPath (@($config,$manifest) + $originals)|Select-Object Path,Hash
  Write-Output "$tag in-memory API PASS:Q=$($score.Q),Q_JOD=$($score.Q_JOD)"
 }
}
if($records.Count -ne 6){throw 'Incomplete pairs'}
$records|ConvertTo-Json|Set-Content "$root/results.json"
$hashes+=Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash
$hashes|Sort-Object Path -Unique|ConvertTo-Json|Set-Content "$root/hashes.json"
Set-Content "$root/PASS.txt" 'Six original-image in-memory API pairs,81 stage checks each plus one score-only same-instance repeat:487 checks/1966412 words bitwise exact. No intermediate file IO; CPU model/NCSF and synchronous host transfers remain.30ppd age24 D6580x96/96x80 only.'
