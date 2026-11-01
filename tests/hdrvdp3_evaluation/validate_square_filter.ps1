param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$ForwardShader,
    [Parameter(Mandatory=$true)][string]$InverseShader,
    [Parameter(Mandatory=$true)][string]$IOShader,
    [Parameter(Mandatory=$true)][string]$MapShader,
    [Parameter(Mandatory=$true)][string]$CasesFile,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$ResultsOnly,
    [switch]$Visual,
    [switch]$SPYR,
    [string]$SPYRFMA,[string]$SPYRSeparate,[string]$MeanShader,[string]$DifferenceShader,[string]$ResidualShader
)
$ErrorActionPreference='Stop';$exe=(Resolve-Path $Executable).Path
$cases=Get-Content $CasesFile -Raw|ConvertFrom-Json
if($cases.Count -ne 6){throw 'Six named cases required'}
if(!$ResultsOnly){if(Test-Path $OutputDirectory){throw 'Fresh output required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$out=(Resolve-Path $OutputDirectory).Path;$records=@()
if($SPYR){$Visual=$true;foreach($shader in @($SPYRFMA,$SPYRSeparate,$MeanShader,$DifferenceShader,$ResidualShader)){if(!$shader-or!(Test-Path $shader)){throw 'Missing SPYR assets'}}}
$expectedStages=if($SPYR){43}elseif($Visual){32}else{26};$expectedValues=if($SPYR){1487387}elseif($Visual){1419272}else{1382400}
foreach($case in $cases){
    $name=$case.name
    if($name -notmatch '^[a-z_]+$'){throw 'Invalid case name'}
    if(!$ResultsOnly){
        $ErrorActionPreference='Continue'
        $arguments=@($case.native,$ForwardShader,$InverseShader,$IOShader,$MapShader,"$out/$name")
        if($Visual){if(!$case.visual){throw "Missing visual goldens:$name"};$arguments+=$case.visual}
        if($SPYR){if(!$case.spyr){throw "Missing SPYR goldens:$name"};$arguments+=@($SPYRFMA,$SPYRSeparate,$MeanShader,$DifferenceShader,$ResidualShader,$case.spyr)}
        & $exe @arguments *> "$out/$name.log"
        $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
        if($code -ne 0){throw "Square connected filter failure:$name"}
    }
    if(!(Test-Path "$out/$name/PASS.txt")){throw "Missing PASS:$name"}
    if((Get-Content "$out/$name.log" -Raw)-match 'VUID-|Validation Error|\[ERROR\]'){throw "Vulkan errors:$name"}
    $rows=Import-Csv "$out/$name/results.tsv" -Delimiter "`t"
    if($rows.Count -ne $expectedStages){throw 'Incomplete stage coverage'}
    $n=0
    foreach($row in $rows){
        if([int]$row.numeric_errors -ne 0-or[int]$row.bit_errors -ne 0){throw "Strict bit failure:$name/$($row.stage)"}
        $n += [int]$row.values
    }
    if($n -ne $expectedValues){throw 'Incomplete values'}
    $records += [pscustomobject]@{case=$name;stages=$expectedStages;values=$n;bit_errors=0}
    $records|ConvertTo-Json|Set-Content "$out/results.json"
    Write-Output "${name}:$expectedStages stages/$expectedValues words bitwise PASS"
}
if(@($records.case|Sort-Object -Unique).Count-ne 6){throw 'Duplicate cases'}
$hashPaths=@($exe,$ForwardShader,$InverseShader,$IOShader,$MapShader,$CasesFile,$PSCommandPath)
if($SPYR){$hashPaths+=@($SPYRFMA,$SPYRSeparate,$MeanShader,$DifferenceShader,$ResidualShader)}
Get-FileHash -LiteralPath $hashPaths|
    Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/hashes.json"
Set-Content "$out/PASS.txt" "Six96x96 original-image pairs;$($expectedStages*6) stage checks/$($expectedValues*6) words bitwise including signed zero;MTF ->GPU L+M ->adaptation;visualP/log=$Visual;SPYR=$SPYR;no CPU intermediate arithmetic;stage-boundary host copies;no Vulkan errors. Not full96x96 metric acceptance."
