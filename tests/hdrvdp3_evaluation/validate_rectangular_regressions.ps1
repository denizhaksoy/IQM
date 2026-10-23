param(
 [Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$BoundedAssets,
 [Parameter(Mandatory=$true)][string]$Prior128,
 [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
$assets=(Resolve-Path -LiteralPath $BoundedAssets).Path
$prior=(Resolve-Path -LiteralPath $Prior128).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh regression output required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$records=@()
foreach($side in @(32,64,128)){
 if($side -eq 128){
  $golden="$prior/public_release/full128_localized";$scoreFile="$prior/full128_localized/quality.txt"
  $ref="$prior/native_quality128_localized/reference.pfm";$test="$prior/native_quality128_localized/test.pfm"
  $fft="$prior/runtime128/fft.spv";$map="$prior/runtime128/map.spv";$base="$prior/runtime128/base.spv"
 }else{
  $golden="$assets/golden$side";$scoreFile="$golden/quality.txt"
  $ref="$assets/input/reference$side.pfm";$test="$assets/input/test${side}_localized.pfm"
  $fft="$assets/fft$side.spv";$map="$assets/map.spv";$base="$assets/base.spv"
 }
 $scores=((Get-Content -LiteralPath $scoreFile)[-1].Trim() -split '\s+')
 $ErrorActionPreference='Continue'
 & $exe $ref $test $fft "$assets/math" $map $base "$assets/spectral" "$root/size$side" $scores[1] $scores[2] $scores[0] *> "$root/size$side.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 Get-Content "$root/size$side.log" -Tail 1
 if($code -or (Select-String -LiteralPath "$root/size$side.log" -Pattern 'VUID-|Validation Error|\[ERROR\]')){throw "Public regression failed:$side"}
 $same=0;foreach($file in Get-ChildItem "$root/size$side" -Filter '*.txt'){
  if($file.Name -in @('input_source.txt','pattern.txt','test_transform.txt','map_shader.txt','base_fft_shader.txt','fft_math_shader_dir.txt')){continue}
  if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath "$golden/$($file.Name)").Hash){throw "Changed numeric export:$side/$($file.Name)"}
  ++$same
 }
 $expected=if($side -eq 32){96}else{107};if($same -ne $expected){throw 'Incomplete regression exports'}
 $records+=[ordered]@{side=$side;numeric_export_hashes_equal=$same;native_scores_bitwise_equal=$true}
 $records|ConvertTo-Json|Set-Content "$root/results.json"
}
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
Set-Content "$root/PASS.txt" 'Current public32/64/128 API score bits and96/107/107 numerical exports unchanged.'
