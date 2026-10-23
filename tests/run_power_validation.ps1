param(
    [Parameter(Mandatory=$true)][string]$PowerShader,
    [string[]]$ExportLists=@(),
    [string]$Octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root ('validation/power_validation_'+[DateTime]::UtcNow.Ticks)
New-Item -ItemType Directory -Path $output | Out-Null
$PowerShader=(Resolve-Path -LiteralPath $PowerShader).Path
$exe=Join-Path $root 'validation/fft_publish_build/Release/IQM-fft-codelet-test.exe'
Get-FileHash -LiteralPath $PowerShader,$exe | Select-Object Path,Hash | ConvertTo-Json | Set-Content (Join-Path $output 'binary_hashes.json')
$cases=@(@{name='random'; script='generate_float_power_reference.m'; extra=@()},
    @{name='boundaries'; script='generate_power_boundary_reference.m'; extra=@()})
$index=0
foreach($list in $ExportLists) {
    foreach($line in Get-Content -LiteralPath $list) {
        $parts=$line -split "`t"
        if($parts.Count -ne 2) { throw "Invalid export-list entry: $line" }
        $cases+=@{name=$parts[0]; script='generate_float_power_reference.m'; extra=@($parts[1],('--seed='+(71441+$index)))}
        ++$index
    }
}
foreach($case in $cases) {
    $fixture=Join-Path $output $case.name
    $extra=$case.extra
    $ErrorActionPreference='Continue'
    & $Octave --quiet (Join-Path $PSScriptRoot $case.script) $fixture @extra *> (Join-Path $output ($case.name+'_reference.log'))
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code -ne 0) { throw "Power reference failed: $($case.name)" }
    $ErrorActionPreference='Continue'
    & $exe $fixture $PowerShader --float-math *> (Join-Path $output ($case.name+'_gpu.log'))
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code -ne 0) { Get-Content (Join-Path $output ($case.name+'_gpu.log')) -Tail 12; throw "Power parity failed: $($case.name) exit=$code" }
    Write-Output "$($case.name) PASS"
}
Set-Content (Join-Path $output 'PASS.txt') "$($cases.Count) datasets, $($cases.Count*65536) tested pairs; both output components repeat the same power function. All numerically exact."
Set-Content (Join-Path $root 'validation/power_validation_latest_path.txt') $output
Write-Output $output
