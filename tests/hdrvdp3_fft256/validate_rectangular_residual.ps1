param(
 [Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$NativeFixture,
 [Parameter(Mandatory=$true)][string]$ReductionShader,
 [Parameter(Mandatory=$true)][string]$SelectionShader,
 [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
$native=(Resolve-Path -LiteralPath $NativeFixture).Path
$reduction=(Resolve-Path -LiteralPath $ReductionShader).Path
$selection=(Resolve-Path -LiteralPath $SelectionShader).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$jobs=@(
 @('reductions128',67072,'--base-reductions',$native,$reduction,"$root/reductions128",128),
 @('chain8x16',328192,'--rectangular-reductions-chain',$native,$reduction,$selection,"$root/chain8x16",16),
 @('chain16x8',328192,'--rectangular-reductions-chain',$native,$reduction,$selection,"$root/chain16x8",8)
)
$records=@()
foreach($job in $jobs){
 $tag=$job[0];$arguments=$job[2..($job.Count-1)]
 $ErrorActionPreference='Continue'; & $exe @arguments *> "$root/$tag.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 Get-Content -LiteralPath "$root/$tag.log" -Tail 3
 $log=[IO.File]::ReadAllText("$root/$tag.log")
 if($code -or $log -notmatch "values=$($job[1]) mismatches=0" -or $log -match 'VUID-|Validation Error|\[ERROR\]'){
  throw "Residual validation failed:$tag;exit=$code;exports retained"
 }
 $records+=[ordered]@{case=$tag;compared_values=$job[1];numeric_mismatches=0}
 $records|ConvertTo-Json|Set-Content "$root/records.json"
}
$files=@($exe,$reduction,$selection,$PSCommandPath)
$files+=@(Get-ChildItem -LiteralPath $native -File|ForEach-Object {$_.FullName})
Get-FileHash -LiteralPath $files|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/provenance.json"
[ordered]@{scope='Isolated128-sample reductions andtwo rectangular DC/FFT/magnitude/first-max chains';reduction_cases=512;reduction_values=67072;chains=2;chain_cases_each=512;chain_values_each=328192;numeric_mismatches=0;precision='Float32';input_layout='row-major,interleaved complex afterGPU DC';padding='none';FFT_normalization=1;DC_processing='native mean removal;onlyselection magnitudeDC zeroed;rawFFT preserved';native_profile='Octave single,3threads,estimate';residual_exponents=@(-90,100);maximum_input_exponent_span=16;log_domain=@(-5,5);CPU_intermediate_replacement=$false;signed_zero_bits_asserted=$false;production_MapGPU_integration=$false}|ConvertTo-Json|Set-Content "$root/COMPLETED.json"
Set-Content "$root/PASS.txt" '512reductions andtwo512-case GPU DC/FFT/magnitude/first-max chains pass exact numeric comparisons.'
