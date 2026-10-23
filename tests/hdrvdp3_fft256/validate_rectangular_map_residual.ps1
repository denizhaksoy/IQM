param([Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$FixtureRoot,
 [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path;$fixture=(Resolve-Path -LiteralPath $FixtureRoot).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$legacy=(Resolve-Path 'validation/asset_generation_639281159480285385/assets_verified').Path
$older=(Resolve-Path 'validation/base_reductions_639281814095504643').Path
$map="$fixture/map15/map15.spv";$map14="$fixture/map15/profile14/map128.spv";$base="$fixture/base85/rectangular_residual.spv"
$jobs=@(
 @('reductions128',$map,"$fixture/native128",'reductions128',15,66560),
 @('residual8x16',$map,"$fixture/native128",'residual8x16',15,32832),
 @('residual16x8',$map,"$fixture/native128",'residual16x8',15,32832),
 @('legacy64',"$legacy/map.spv","$older/native64",'reductions64',13,33792),
 @('legacy256',$map14,"$older/native256",'reductions256',14,132096),
 @('profile15_64',$map,"$older/native64",'reductions64',15,33792),
 @('profile15_256',$map,"$older/native256",'reductions256',15,132096)
)
$records=@()
foreach($job in $jobs){
 $tag=$job[0];$ErrorActionPreference='Continue'
 & $exe $job[1] $base $job[2] $job[3] $job[4] "$root/$tag" *> "$root/$tag.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop';Get-Content "$root/$tag.log" -Tail 2
 $log=[IO.File]::ReadAllText("$root/$tag.log")
 if($code -or $log -notmatch "float_values=$($job[5]) mismatches=0" -or $log -match 'VUID-|Validation Error|\[ERROR\]'){throw "Map residual failed:$tag"}
 $records+=[ordered]@{case=$tag;profile=$job[4];float_values=$job[5];numeric_mismatches=0}
 $records|ConvertTo-Json|Set-Content "$root/records.json"
}
$guardJobs=@(
 @('legacy_contracts',12,'--contracts',"$legacy/map.spv",$map14,"$legacy/base.spv","$root/legacy_contracts"),
 @('rectangular_contracts',16,'--contracts-rectangular',"$legacy/map.spv",$map14,$map,
  (Resolve-Path 'validation/spyr_dimensions_639282601672947206/rectangular_base_compiled/rectangular_base.spv').Path,"$root/rectangular_contracts")
)
foreach($job in $guardJobs){
 $tag=$job[0];$arguments=$job[2..($job.Count-1)];$ErrorActionPreference='Continue'
 & $exe @arguments *> "$root/$tag.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 Get-Content "$root/$tag.log" -Tail 1;$log=[IO.File]::ReadAllText("$root/$tag.log")
 if($code -or $log -notmatch "contracts rejected=$($job[1])" -or $log -match 'VUID-|Validation Error|\[ERROR\]'){throw "Map contract failed:$tag"}
}
Get-FileHash -LiteralPath $exe,$map,$map14,$base,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
[ordered]@{scope='Actual MapGPU residual APIs;not full rectangular quality';map_ABI=15;base_FFT_ABI=85;new_domain_compared_values=132224;regression_compared_values=331776;numeric_mismatches=0;reduction_cases128=512;rectangular_cases_each=64;contracts_rejected=28;precision='Float32';layout='row-major interleaved,explicit8x16/16x8';padding='none';normalization=1;DC_processing='GPU native mean removal;selection magnitude DC zero only';API_intermediate_GPU_readbacks=$true;native_stage_substitution=$false;signed_zero_bits_asserted=$false}|ConvertTo-Json|Set-Content "$root/COMPLETED.json"
Set-Content "$root/PASS.txt" 'MapGPU ABI15/85 residual128 andlegacy13/14 regressions exact;28 contract rejections pass.'
