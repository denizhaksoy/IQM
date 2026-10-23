param([Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$NativeFixture,
 [Parameter(Mandatory=$true)][string]$MapShader,
 [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path;$native=(Resolve-Path -LiteralPath $NativeFixture).Path;$shader=(Resolve-Path -LiteralPath $MapShader).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$records=@()
foreach($mode in @('rect-masking','rect-float','rect-double')){
 $ErrorActionPreference='Continue';& $exe $shader $shader $native $mode 16 "$root/$mode" *> "$root/$mode.log"
 $code=$LASTEXITCODE;$ErrorActionPreference='Stop';Get-Content "$root/$mode.log" -Tail 6
 $log=[IO.File]::ReadAllText("$root/$mode.log")
 $pattern=switch($mode){'rect-masking'{'float_values=696320 mismatches=0'} 'rect-float'{'float_values=487424 mismatches=0'} 'rect-double'{'double_values=174080 double_bit_mismatches=0'}}
 $pass=$code -eq 0 -and $log -match $pattern -and $log -notmatch 'VUID-|Validation Error|\[ERROR\]'
 $records+=[ordered]@{case=$mode;exit=$code;pass=$pass};$records|ConvertTo-Json|Set-Content "$root/records.json"
}
Get-FileHash -LiteralPath $exe,$shader,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
if(@($records|Where-Object {!$_.pass}).Count){throw 'Rectangular map checks failed;all diagnostic exports preserved'}
[ordered]@{scope='Isolated actual MapGPU ABI16 rectangular masking/resize;not full quality';mask_cases=128;mask_float_values=696320;float_resize_cases=160;float_resize_values=487424;binary_resize_cases=64;binary64_bitwise_values=174080;numeric_mismatches=0;precision='Float32 masks/resize;binary64 mask resize compared bybits';layout='row-major';padding='pool zero outside;imresize bicubic/antialias/symmetric';DC_processing='none';signed_zero_float_bits_asserted=$false;binary64_bits_asserted=$true}|ConvertTo-Json|Set-Content "$root/COMPLETED.json"
Set-Content "$root/PASS.txt" 'Rectangular masking/Float32 resize exact;binary64 mask resize bitwise exact.'
