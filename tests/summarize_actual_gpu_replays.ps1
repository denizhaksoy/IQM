param([Parameter(Mandatory=$true)][string]$RunsManifest,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$runs=Get-Content -LiteralPath $RunsManifest -Raw|ConvertFrom-Json
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh summary directory required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
$records=@();$modelHashes=$null
foreach($property in $runs.PSObject.Properties){
 $pattern=$property.Name;$root=$property.Value
 foreach($stage in @('PASS.txt','prepare/PASS.txt','jnd/PASS.txt','spyr/PASS.txt','pool/PASS.txt','comparison/PASS.txt','quality/PASS.txt','validated/PASS.txt')){if(!(Test-Path -LiteralPath "$root/$stage")){throw "Missing $pattern/$stage"}}
 foreach($shape in @(@(80,96),@(96,80))){foreach($mode in @('mtf','la')){foreach($label in @('ref','test')){$suffix=if($mode -eq 'la'){'_from_gpu'}else{''};$case="gpu_h$($shape[0])_w$($shape[1])_${mode}_${label}${suffix}";if(!(Test-Path -LiteralPath "$root/filters/$case/PASS.txt")){throw "Missing filter $case"}}}}
 $data=Get-Content -LiteralPath "$root/validated/results.json" -Raw|ConvertFrom-Json
 if(!$data.original_filter_and_comparison_inputs_bit_identical -or !$data.fixed_matrix_and_JND_tables_identical_across_all_images){throw 'Input/model provenance failed'}
 $current=$data.fixed_model_table_hashes|ConvertTo-Json -Compress
 if($null -eq $modelHashes){$modelHashes=$current}elseif($current -ne $modelHashes){throw 'Fixed model tables changed across content'}
 if($data.records.Count -ne 2){throw 'Missing orientation'}
 foreach($row in $data.records){
  if(!$row.score_bit_identity){throw 'Score identity failed'}
  $path="$root/quality/h$($row.shape[0])_w$($row.shape[1])_quality_scores.f32"
  $bytes=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $path).Path)
  if($bytes.Length -ne 8 -or [BitConverter]::ToUInt32($bytes,0) -ne [uint32]$row.score_bits[0] -or [BitConverter]::ToUInt32($bytes,4) -ne [uint32]$row.score_bits[1]){throw 'Score dump differs from validated bits'}
  $records+=[pscustomobject]@{pattern=$pattern;h=$row.shape[0];w=$row.shape[1];Q=$row.Q;Q_JOD=$row.Q_JOD;score_bits=$row.score_bits;root=$root}
 }
}
if($records.Count -ne 6){throw 'Expected three contents x two orientations'}
@{scope='Six experimental file-composed Float32 luminance quality pairs;30ppd age24 D65;no public support change';records=$records;fixed_model_tables_identical=$true}|ConvertTo-Json -Depth 6|Set-Content "$OutputDirectory/SUMMARY.json"
Set-Content "$OutputDirectory/PASS.txt" 'Six image-to-score replays pass all stages; native final score bits and input/model provenance verified.'
$records|Select-Object pattern,h,w,Q,Q_JOD|Format-Table -AutoSize
