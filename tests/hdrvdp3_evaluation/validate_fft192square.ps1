param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$ForwardShader,
    [Parameter(Mandatory=$true)][string]$InverseShader,
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$ResultsOnly
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path $Executable).Path
if (!$ResultsOnly) {
    if (Test-Path $OutputDirectory) { throw 'Fresh square FFT outputs required' }
    New-Item -ItemType Directory $OutputDirectory | Out-Null
}
$out=(Resolve-Path $OutputDirectory).Path;$records=@()
foreach ($pattern in @('asymmetric','impulse','hdr_ramp')) {
    foreach ($label in @('ref','test')) { foreach ($chain in @('mtf','la')) { foreach ($direction in @('forward','inverse')) {
        $tag="${pattern}_${label}_${chain}_${direction}"
        $shader=if ($direction -eq 'forward') { $ForwardShader } else { $InverseShader }
        if (!$ResultsOnly) {
            $ErrorActionPreference='Continue'
            & $exe "$NativeDirectory/native_$pattern" $shader $label "$out/$tag" "--${direction}192x192" $chain *> "$out/$tag.log"
            $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
            if ($code -ne 0) { throw "Square FFT failure: $tag" }
        }
        if (!(Test-Path "$out/$tag/PASS.txt")) { throw "Missing PASS: $tag" }
        $summary=(Get-Content "$out/$tag/strict_results.txt" -Raw).Trim()
        if ($summary -ne 'values=73728 numeric_errors=0 bit_errors=0') { throw "Strict result: $tag/$summary" }
        if ((Get-Content "$out/$tag.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]') { throw "Vulkan errors: $tag" }
        $records += [pscustomobject]@{case=$tag;values=73728;numeric_errors=0;bit_errors=0}
        $records | ConvertTo-Json | Set-Content "$out/results.json"
        Write-Output "$tag bitwise PASS"
    } } }
}
if ($records.Count -ne 24) { throw 'Incomplete square FFT coverage' }
Get-FileHash -LiteralPath $exe,$ForwardShader,$InverseShader,$PSCommandPath |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Set-Content "$out/PASS.txt" '24 staged192x192 FFT runs;1769472 Float32 words bitwise exact including signed zero;native input/filtered-spectrum and normalization preserved;no Vulkan errors. Isolated FFT acceptance,not full96x96 metric acceptance.'
