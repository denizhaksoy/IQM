param(
    [Parameter(Mandatory=$true)][string]$NativeDirectory,
    [Parameter(Mandatory=$true)][string]$PlanExecutable,
    [Parameter(Mandatory=$true)][string]$GPUExecutable,
    [Parameter(Mandatory=$true)][string]$Shader,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
$planText=Get-Content "$NativeDirectory/verified_plans_h96_w80_asymmetric/plans.txt" -Raw
$selected=[regex]::Match($planText,'(?s)ref_mtf_t3_unaligned_outplace forward\s+(.*?)(?=ref_mtf_t3_unaligned_outplace inverse)')
if(!$selected.Success){throw 'Missing selected rank2 plan'}
$rank2=$selected.Groups[1].Value -replace '\s',''
$rows=@();$hashes=@();$firstPlan=$null
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){foreach($label in @('ref','test')){foreach($chain in @('mtf','la')){
    $key="${pattern}_${label}_${chain}"
    $input=Join-Path $NativeDirectory "native_h96_w80_$pattern/${label}_${chain}_stage0.txt"
    $native=Join-Path $OutputDirectory "native_$key"
    $ErrorActionPreference='Continue'
    & $PlanExecutable --forward192-axis $input $native > "$OutputDirectory/native_$key.log" 2>&1
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code){throw "Native oracle failed:$key exit=$code"}
    $plan=(Get-Content "$native/axis0_plan.txt" -Raw) -replace '\s',''
    if(!$rank2.Contains($plan)){throw "Isolated plan is not a subtree of prior rank2 candidate:$key"}
    if($null -eq $firstPlan){$firstPlan=$plan}elseif($plan -ne $firstPlan){throw 'Native axis plans differ'}
    @{input=(Resolve-Path $input).Path;pattern=$pattern;label=$label;chain=$chain}|ConvertTo-Json|Set-Content "$native/source.json"
    $hashes+=Get-FileHash $input
    $ErrorActionPreference='Continue'
    & $GPUExecutable $native $Shader ref "$OutputDirectory/gpu_$key" --forward192-axis > "$OutputDirectory/gpu_$key.log" 2>&1
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code){throw "GPU axis comparison failed:$key exit=$code"}
    $result=(Select-String -LiteralPath "$OutputDirectory/gpu_$key.log" -Pattern '^forward_axis').Line
    if($result -ne 'forward_axis values=31040 mismatches=0 max=0'){throw "Missing exact result:$key"}
    $rows+="$key`t$result"
    $rows|Set-Content "$OutputDirectory/results.tsv"
    Write-Output "$key forward-axis=0 mismatches"
}}}
$hashes+=Get-FileHash $PlanExecutable,$GPUExecutable,$Shader
$hashes|Format-List|Out-File "$OutputDirectory/input_binary_hashes.txt"
'12 streams; 372480 unnormalized real192 axis Float32 values numerically exact; signed-zero bits not asserted; all isolated plans match prior rank2 subtree.'|Set-Content "$OutputDirectory/PASS"
