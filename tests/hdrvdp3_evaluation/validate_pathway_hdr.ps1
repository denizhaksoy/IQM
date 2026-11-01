param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Runtime,
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$ResultsOnly
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path $Executable).Path
$native=(Resolve-Path $NativeDirectory).Path
$assets=(Resolve-Path $Runtime).Path
if (!$ResultsOnly) {
    if (Test-Path $OutputDirectory) { throw 'Fresh HDR outputs required' }
    New-Item -ItemType Directory $OutputDirectory | Out-Null
}
$out=(Resolve-Path $OutputDirectory).Path
$records=@()
foreach ($shape in @('h80_w96','h96_w80')) {
    if (!$ResultsOnly) {
        $ErrorActionPreference='Continue'
        & $exe "$native/$shape" $assets "$out/$shape" *> "$out/$shape.log"
        $code=$LASTEXITCODE
        $ErrorActionPreference='Stop'
        if ($code -ne 0) { throw "HDR comparison failed: $shape" }
    }
    if (!(Test-Path "$out/$shape/PASS.txt")) { throw "Missing PASS: $shape" }
    if ((Get-Content "$out/$shape.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]') {
        throw "Vulkan errors: $shape"
    }
    $rows=@(Import-Csv "$out/$shape/results.tsv" -Delimiter "`t")
    if ($rows.Count -ne 69) { throw "Incomplete stages: $shape" }
    $values=0
    foreach ($row in $rows) {
        if ([int]$row.numeric_errors -ne 0 -or [int]$row.bit_errors -ne 0) {
            throw "Strict parity failure: $shape"
        }
        $values += [int]$row.values
    }
    if ($values -ne 237733) { throw "Incomplete values: $shape" }
    $scores=Get-Content "$out/$shape/scores.json" -Raw | ConvertFrom-Json
    $records += [pscustomobject]@{
        shape=$shape; stages=69; values=$values; bit_errors=0
        Q=$scores.Q; Q_JOD=$scores.Q_JOD
    }
}
$records | ConvertTo-Json | Set-Content "$out/results.json"
Get-FileHash -LiteralPath $exe,$PSCommandPath,"$assets/pathway.runtime","$native/metadata.json" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Set-Content "$out/PASS.txt" '2 HDR-derived grayscale pairs;138 checks/475466 values bitwise exact;no Vulkan errors'
