param(
 [Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$NativeRoot,
 [Parameter(Mandatory=$true)][string]$ForwardShader,
 [Parameter(Mandatory=$true)][string]$InverseShader,
 [Parameter(Mandatory=$true)][string]$MathDirectory,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [switch]$ThroughLocalAdapt
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
$exe=(Resolve-Path -LiteralPath $Executable).Path
$native=(Resolve-Path -LiteralPath $NativeRoot).Path
$forward=(Resolve-Path -LiteralPath $ForwardShader).Path
$inverse=(Resolve-Path -LiteralPath $InverseShader).Path
$math=(Resolve-Path -LiteralPath $MathDirectory).Path
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$records=@()
foreach($shape in @('h64_w128','h128_w64')){foreach($pattern in @('asymmetric','impulse','hdr_ramp')){
 $fixture="$native/native_${shape}_$pattern"
 foreach($label in @('ref','test')){foreach($mode in @('mtf','la')){
  $tag="${shape}_${pattern}_${label}_$mode"
  $ErrorActionPreference='Continue'
  if($ThroughLocalAdapt){
   & $exe $fixture $forward $math $label $mode "$root/$tag" *> "$root/$tag.log"
  }else{
   & $exe $fixture $forward $inverse $math $label $mode "$root/$tag" --rectangular *> "$root/$tag.log"
  }
  $code=$LASTEXITCODE
  $ErrorActionPreference='Stop'
  Get-Content -LiteralPath "$root/$tag.log" -Tail 2
  $log=[IO.File]::ReadAllText("$root/$tag.log")
  $gpuPattern=if($ThroughLocalAdapt){'LocalAdaptFFT GPU values=270336 mismatches=0'}else{'filter_chain GPU values=270336 mismatches=0'}
  $filterPattern=if($ThroughLocalAdapt){'host_filter values=32768 mismatches=0'}else{'filter float32 values=32768 mismatches=0'}
  if($code -or $log -notmatch $gpuPattern -or $log -notmatch $filterPattern){
   throw "Rectangular filter failed:$tag;exit=$code;exports retained"
  }
  if($log -match 'VUID-|Validation Error|\[ERROR\]'){throw "Vulkan diagnostic:$tag"}
  $records+=[ordered]@{case=$tag;gpu_values=270336;gpu_numeric_mismatches=0;filter_float_values=32768;filter_float_mismatches=0}
  $records|ConvertTo-Json|Set-Content "$root/records.json"
 }}
}}
$files=@($exe,$forward,$inverse,$PSCommandPath)
foreach($name in @('prepare','multiply','finish')){$files+=Join-Path $math "$name.spv"}
Get-FileHash -LiteralPath $files|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/binary_and_driver_hashes.json"
[ordered]@{cases=24;gpu_values=6488064;filter_float_values=786432;numeric_mismatches=0;backend=$(if($ThroughLocalAdapt){'Production LocalAdaptFFT'}else{'Isolated test chain'});scope='Rectangular MTF/LA components;not continuous visual pathway orquality';input='R32 Float32 images64x128/128x64';ppd=30;precision='Float32 GPU;host filters checked afterFloat32 rounding';padding='MTF symmetric post;LA replicate post';FFT_layout='column-major interleaved;explicit filter transpose';export_layout=$(if($ThroughLocalAdapt){'trace stages0..3 column-major;output/filter row-major'}else{'stage0/2 column-major;stage1/3 row-major'});inverse_normalization='once1/32768';DC_processing='none';CPU_intermediate_replacement=$false;signed_zero_bits_asserted=$false}|ConvertTo-Json|Set-Content "$root/COMPLETED.json"
Set-Content "$root/PASS.txt" '24rectangular MTF/LA chains;all five GPU stages andFloat32 filter checks exact.'
