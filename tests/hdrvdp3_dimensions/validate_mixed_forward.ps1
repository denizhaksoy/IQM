param(
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$AxisDirectory,
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Shader,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
$rows=@();$hashes=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){foreach($label in @('ref','test')){foreach($chain in @('mtf','la')){
    $key="${pattern}_${label}_${chain}"
    $source=Join-Path $NativeDirectory "native_h80_w96_$pattern"
    $fixture=Join-Path $OutputDirectory "fixture_$key"
    New-Item -ItemType Directory $fixture|Out-Null
    $input=Join-Path $source "${label}_${chain}_stage0.txt"
    $expected=Join-Path $source $(if($chain -eq 'mtf'){"${label}_native_forward.txt"}else{"${label}_la_native_forward.txt"})
    Copy-Item -LiteralPath $input -Destination "$fixture/ref_mtf_stage0.txt"
    Copy-Item -LiteralPath $expected -Destination "$fixture/ref_native_forward.txt"
    @{input=(Resolve-Path $input).Path;expected=(Resolve-Path $expected).Path;label=$label;chain=$chain;pattern=$pattern}|ConvertTo-Json|Set-Content "$fixture/source.json"
    $hashes+=Get-FileHash $input,$expected
    $ErrorActionPreference='Continue'
    & $Executable $fixture $Shader ref "$OutputDirectory/gpu_$key" --forward160x192 > "$OutputDirectory/gpu_$key.log" 2>&1
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code -ne 0){throw "GPU comparison failed: $key exit=$code"}
    $result=(Select-String -LiteralPath "$OutputDirectory/gpu_$key.log" -Pattern '^forward values=').Line
    if($result -ne 'forward values=61440 mismatches=0'){throw "Missing exact full-forward result: $key"}
    $actual=@(Get-Content "$OutputDirectory/gpu_$key/axis0_storage.txt"|Select-Object -Skip 1)
    $axis=@(Get-Content "$AxisDirectory/native_$key/axis0_compact.txt"|Select-Object -Skip 1)
    if($actual.Count -ne 31104 -or $axis.Count -ne 31104){throw 'Invalid compact axis count'}
    $mismatches=0;for($i=0;$i -lt $axis.Count;++$i){if([single]$actual[$i] -ne [single]$axis[$i]){++$mismatches}}
    if($mismatches){throw "First axis regression: $key mismatches=$mismatches"}
    $rows+="$key`t61440`t0`t31104`t0"
    $rows|Set-Content "$OutputDirectory/results.tsv"
    Write-Output "$key full-forward=0 first-axis=0 mismatches"
}}}
$hashes+=Get-FileHash $Executable,$Shader
$hashes|Format-List|Out-File "$OutputDirectory/input_binary_hashes.txt"
'12 unique streams; 737280 full-forward Float32 values exact; 373248 first-axis values exact. Numerical equality; signed-zero bits not asserted.'|Set-Content "$OutputDirectory/PASS"
