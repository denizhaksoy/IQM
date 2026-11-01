param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$AssetsDirectory,
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$ResultsOnly
)
$ErrorActionPreference='Stop';$exe=(Resolve-Path $Executable).Path
if(!$ResultsOnly){if(Test-Path $OutputDirectory){throw 'Fresh quality output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$out=(Resolve-Path $OutputDirectory).Path;$records=@()
foreach($name in @('asymmetric','impulse','hdr_ramp','constant','identity','real')){
    if(!$ResultsOnly){
        $ErrorActionPreference='Continue'
        & $exe "$NativeDirectory/native_$name/h96_w96" $AssetsDirectory "$out/$name" *> "$out/$name.log"
        $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
        if($code-ne 0){throw "Square quality failure:$name"}
    }
    if(!(Test-Path "$out/$name/PASS.txt")){throw "Missing PASS:$name"}
    if((Get-Content "$out/$name.log" -Raw)-match 'VUID-|Validation Error|\[ERROR\]'){throw "Vulkan errors:$name"}
    $rows=Import-Csv "$out/$name/results.tsv" -Delimiter "`t"
    if($rows.Count-ne 69){throw 'Incomplete quality stages'}
    $n=0;foreach($row in $rows){if([int]$row.numeric_errors-ne 0-or[int]$row.bit_errors-ne 0){throw "Strict mismatch:$name/$($row.stage)"};$n+=[int]$row.values}
    if($n-ne 285277){throw 'Incomplete quality values'}
    $scores=Get-Content "$out/$name/scores.json" -Raw|ConvertFrom-Json
    if($name-eq 'identity'-and($scores.Q-ne 10-or$scores.Q_JOD-ne 10)){throw 'Identity score contract'}
    $records+=[pscustomobject]@{case=$name;stages=69;values=$n;bit_errors=0;Q=$scores.Q;Q_JOD=$scores.Q_JOD}
    $records|ConvertTo-Json|Set-Content "$out/results.json"
    Write-Output "${name}:69 checks/285277 values bitwise PASS"
}
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/hashes.json"
Get-ChildItem $AssetsDirectory -File|Get-FileHash|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/asset_hashes.json"
Set-Content "$out/PASS.txt" 'Six original96x96 pairs;414 checks/1711662 Float32/Float64 values bitwise including Q/Q_JOD,band errors and base noise;no Vulkan errors. Private in-memory diagnostic profile,not public CLI/runtime96x96 acceptance.'
