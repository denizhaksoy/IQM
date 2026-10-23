param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh checkpoint required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$workspace=(Get-Location).Path
$prior=Join-Path $workspace 'validation/rectangular_maps_639282910800030657'
$exe="$prior/validated_binaries/IQM-quality-rectangular-diagnostic.exe"
$octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe'
$records=@()
foreach($shape in @('h64_w128','h128_w64')){
 foreach($pattern in @('impulse','hdr_ramp','asymmetric')){
  $transform=if($pattern -eq 'asymmetric'){'identity'}else{'localized'}
  $name="${shape}_${pattern}_${transform}";$native="$root/native_$name";$export="$root/gpu_$name"
  $source="$workspace/validation/rectangular_filter_639282611436713337/native_${shape}_$pattern"
  $ErrorActionPreference='Continue'
  & $octave --quiet --no-gui tests/generate_rectangular_quality_reference.m $source $native $transform *> "$root/native_$name.log"
  $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
  if($code){throw "Native reference failed:$name"}
  $scores=(Get-Content "$native/scores.txt").Trim() -split '\s+'
  $ErrorActionPreference='Continue'
  & $exe "$native/reference.pfm" "$native/test.pfm" "$workspace/validation/rectangular_filter_639282611436713337/combined/rectangular_reference.spv" "$workspace/validation/map128_api_639281996762386676/runtime128/math" "$prior/shader16_candidate/map16.spv" "$workspace/validation/rectangular_residual_639282893861381197/base85/rectangular_residual.spv" "$workspace/validation/asset_generation_639281159480285385/assets_verified/spectral" $export $scores[0] $scores[1] *> "$root/gpu_$name.log"
  $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
  if($code -or (Select-String -LiteralPath "$root/gpu_$name.log" -Pattern 'VUID-|Validation Error|\[ERROR\]')){throw "GPU diagnostic failed:$name"}
  & "$PSScriptRoot/validate_rectangular_quality.ps1" -ExportDirectory $export -OutputDirectory "$root/stages_$name"
  $records+=[ordered]@{case=$name;Q=$scores[0];Q_JOD=$scores[1];exact_stage_checks=6;score_bits_equal=$true}
  $records|ConvertTo-Json|Set-Content "$root/results.json"
  Write-Output "Completed $name Q=$($scores[0]) Q_JOD=$($scores[1])"
 }
}
Get-FileHash -LiteralPath $exe,$PSCommandPath,'tests/generate_rectangular_quality_reference.m'|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
Set-Content "$root/PASS.txt" 'Six new rectangular full-quality pairs pass score bits and all six exact validators.'
