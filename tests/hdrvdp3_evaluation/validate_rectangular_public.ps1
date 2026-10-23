param([Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$RuntimeDirectory,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [ValidateSet('h64_w128','h128_w64')][string[]]$Shapes=@('h64_w128','h128_w64'))
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
$runtime=(Resolve-Path -LiteralPath $RuntimeDirectory).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh public replay directory required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$workspace=(Get-Location).Path
$maps="$workspace/validation/rectangular_maps_639282910800030657"
$stress="$workspace/validation/rectangular_stress_639282921084448635"
$records=@()
foreach($shape in $Shapes){
 foreach($pattern in @('asymmetric_localized','impulse_localized','hdr_ramp_localized','asymmetric_identity')){
  $name="${shape}_$pattern"
  if($pattern -eq 'asymmetric_localized'){$native="$maps/native_quality_$shape";$golden="$maps/full_fixed_$shape"}
  else{$native="$stress/native_$name";$golden="$stress/gpu_$name"}
  $scores=((Get-Content "$golden/quality.txt")[-1].Trim() -split '\s+')
  $ErrorActionPreference='Continue'
  & $exe "$native/reference.pfm" "$native/test.pfm" "$runtime/fft.spv" "$runtime/math" "$runtime/map.spv" "$runtime/base.spv" "$workspace/validation/asset_generation_639281159480285385/assets_verified/spectral" "$root/$name" $scores[1] $scores[2] $scores[0] *> "$root/$name.log"
  $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
  if($code -or (Select-String -LiteralPath "$root/$name.log" -Pattern 'VUID-|Validation Error|\[ERROR\]')){throw "Public rectangular replay failed:$name"}
  $same=0
  foreach($file in Get-ChildItem "$root/$name" -Filter '*.txt'){
   if($file.Name -in @('input_source.txt','pattern.txt','test_transform.txt','map_shader.txt','base_fft_shader.txt','fft_math_shader_dir.txt')){continue}
   if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath "$golden/$($file.Name)").Hash){throw "Changed numeric export:$name/$($file.Name)"}
   ++$same
  }
  if($same -ne 107){throw "Incomplete public exports:$name/$same"}
  $records+=[ordered]@{case=$name;native_score_bits_equal=$true;numeric_export_hashes_equal=$same}
  $records|ConvertTo-Json|Set-Content "$root/results.json"
  Write-Output "Public rectangular replay exact:$name,$same exports"
 }
}
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
Set-Content "$root/PASS.txt" "$($records.Count) public rectangular pairs preserve native score bits and$($records.Count*107) numerical exports."
