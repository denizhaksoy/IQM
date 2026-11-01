param(
    [Parameter(Mandatory=$true)][string]$ExecutableDirectory,
    [Parameter(Mandatory=$true)][string]$MeanShader,
    [Parameter(Mandatory=$true)][string]$DifferenceShader,
    [Parameter(Mandatory=$true)][string]$FFTShader,
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$ResultsOnly
)
$ErrorActionPreference='Stop'
$meanExe=(Resolve-Path "$ExecutableDirectory/mean144-test.exe").Path
$pipelineExe=(Resolve-Path "$ExecutableDirectory/base144-pipeline-test.exe").Path
$meanManifest=(Resolve-Path "$NativeDirectory/mean_fixtures_complete/manifest.txt").Path
$pipelineManifest=(Resolve-Path "$NativeDirectory/pipeline_fixtures/manifest.txt").Path
if (!$ResultsOnly) {
    if (Test-Path $OutputDirectory) { throw 'Fresh base144 outputs required' }
    New-Item -ItemType Directory $OutputDirectory | Out-Null
}
$out=(Resolve-Path $OutputDirectory).Path
if (!$ResultsOnly) {
    $ErrorActionPreference='Continue'
    & $meanExe $MeanShader $meanManifest "$out/mean" *> "$out/mean.log"
    $meanCode=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if ($meanCode -ne 0) { throw 'Mean144 failure; inspect mean.log' }
    $ErrorActionPreference='Continue'
    & $pipelineExe $MeanShader $DifferenceShader $FFTShader $pipelineManifest "$out/pipeline" --pathway-difference *> "$out/pipeline.log"
    $pipelineCode=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if ($pipelineCode -ne 0) { throw 'Base144 pipeline failure; inspect pipeline.log' }
}
foreach ($name in @('mean','pipeline')) {
    if (!(Test-Path "$out/$name/PASS.txt")) { throw "Missing PASS: $name" }
    if ((Get-Content "$out/$name.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]') { throw "Vulkan errors: $name" }
}
$means=Import-Csv "$out/mean/results.tsv" -Delimiter "`t"
if ($means.Count -ne 11) { throw 'Incomplete mean coverage' }
$nativeValues=0;$syntheticValues=0
foreach ($row in $means) {
    if ([int]$row.numeric_mismatches -ne 0 -or [int]$row.bit_mismatches -ne 0) { throw 'Mean bit mismatch' }
    if ($row.case -eq 'synthetic') { $syntheticValues += [int]$row.values }
    else { if ([int]$row.values -ne 145) { throw 'Incomplete native mean' };$nativeValues += [int]$row.values }
}
if ($nativeValues -ne 1450 -or $syntheticValues -ne 1187840) { throw 'Incomplete mean values' }
$pipeline=Import-Csv "$out/pipeline/results.tsv" -Delimiter "`t"
if ($pipeline.Count -ne 5 -or @($pipeline.case | Sort-Object -Unique).Count -ne 5) { throw 'Incomplete pipeline coverage' }
foreach ($row in $pipeline) {
    foreach ($field in @('mean_bit_errors','pack_bit_errors','fft_numeric_errors','mag_index_bit_errors','fft_zero_sign_differences')) {
        if ([int]$row.$field -ne 0) { throw "Pipeline failure: $($row.case)/$field" }
    }
}
Get-FileHash -LiteralPath $meanExe,$pipelineExe,$MeanShader,$DifferenceShader,$FFTShader,$meanManifest,$pipelineManifest,$PSCommandPath |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Set-Content "$out/PASS.txt" 'Mean144:1187840 bounded synthetic and1450 official words bitwise;five GPU mean/difference/FFT/hypot/index chains:5055 words bitwise including signed zero;no Vulkan errors. This is isolated base-band acceptance,not full96x96 metric acceptance.'
