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
    $source=Join-Path $NativeDirectory "native_h96_w80_$pattern"
    $fixture=Join-Path $OutputDirectory "fixture_$key"
    New-Item -ItemType Directory $fixture|Out-Null
    $input=Join-Path $source "${label}_${chain}_stage2.txt"
    $expected=Join-Path $source $(if($chain -eq 'mtf'){"${label}_native_inverse.txt"}else{"${label}_la_native_inverse.txt"})
    $raw=Join-Path $NativeDirectory "verified_plans_h96_w80_$pattern/${label}_${chain}_inverse_unscaled.txt"
    Copy-Item -LiteralPath $input -Destination "$fixture/ref_mtf_stage2.txt"
    Copy-Item -LiteralPath $expected -Destination "$fixture/ref_native_inverse.txt"
    Copy-Item -LiteralPath $raw -Destination "$fixture/ref_native_inverse_unscaled.txt"
    @{input=(Resolve-Path $input).Path;expected=(Resolve-Path $expected).Path;unscaled=(Resolve-Path $raw).Path}|ConvertTo-Json|Set-Content "$fixture/source.json"
    $hashes+=Get-FileHash $input,$expected,$raw
    $axis=@(Get-Content "$AxisDirectory/native_$key/axis0_expected.txt"|Select-Object -Skip 1)
    if($axis.Count -ne 61440){throw 'Invalid native first-axis count'}
    foreach($mode in @('raw','normalized')){
        $flag=if($mode -eq 'raw'){'--inverse192x160-raw'}else{'--inverse192x160'}
        $out=Join-Path $OutputDirectory "gpu_${key}_$mode"
        $ErrorActionPreference='Continue'
        & $Executable $fixture $Shader ref $out $flag > "$OutputDirectory/gpu_${key}_$mode.log" 2>&1
        $code=$LASTEXITCODE
        $ErrorActionPreference='Stop'
        if($code){throw "GPU inverse comparison failed:$key/$mode exit=$code"}
        $result=(Select-String -LiteralPath "$OutputDirectory/gpu_${key}_$mode.log" -Pattern '^inverse values=').Line
        if($result -ne 'inverse values=61440 mismatches=0'){throw "Missing exact inverse result:$key/$mode"}
        $actual=@(Get-Content "$out/axis0_storage.txt"|Select-Object -Skip 1)
        if($actual.Count -ne 61440){throw 'Invalid GPU first-axis count'}
        $m=0;for($i=0;$i -lt $axis.Count;++$i){if([single]$actual[$i] -ne [single]$axis[$i]){++$m}}
        if($m){throw "First-axis regression:$key/$mode mismatches=$m"}
        $rows+="$key`t$mode`t61440`t0`tfirst-axis=0"
        $rows|Set-Content "$OutputDirectory/results.tsv"
        Write-Output "$key/$mode inverse=0 first-axis=0 mismatches"
    }
}}}
$hashes+=Get-FileHash $Executable,$Shader
$hashes|Format-List|Out-File "$OutputDirectory/input_binary_hashes.txt"
'12 streams; 737280 raw inverse values exact and 737280 normalized inverse values exact. First-axis regressions exact in both modes. Numerical equality; signed-zero bits not asserted.'|Set-Content "$OutputDirectory/PASS"
