param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$NativeFixture,
    [Parameter(Mandatory=$true)][string]$ForwardShader,
    [Parameter(Mandatory=$true)][string]$InverseShader,
    [Parameter(Mandatory=$true)][string]$MathDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
$records=@()
foreach($label in @('ref','test')){foreach($mode in @('mtf','la')){
    $tag=$label+'_'+$mode
    $ErrorActionPreference='Continue'
    & $Executable $NativeFixture $ForwardShader $InverseShader $MathDirectory $label $mode "$OutputDirectory/$tag" *> "$OutputDirectory/$tag.log"
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    $log=[IO.File]::ReadAllText((Join-Path $OutputDirectory "$tag.log"))
    if($code -ne 0 -or $log -notmatch 'filter_chain GPU values=540672 mismatches=0'){
        throw "FFT256 chain failed:$tag exit=$code; see retained log"
    }
    if($log -match '\[ERROR\]|Validation Error|VUID-'){throw "Vulkan validation error:$tag"}
    $records+=[pscustomobject]@{case=$tag;gpu_values=540672;gpu_numeric_mismatches=0;filter_float_values=65536;filter_float_mismatches=0}
    Write-Output "$tag GPU540672/0; filter65536/0"
}}
$records|ConvertTo-Json|Set-Content "$OutputDirectory/records.json"
$hashes=@(Get-FileHash $Executable,$ForwardShader,$InverseShader)
$hashes+=Get-ChildItem -LiteralPath $NativeFixture -File|Get-FileHash
foreach($name in @('prepare','multiply','finish')){$hashes+=Get-FileHash (Join-Path $MathDirectory "$name.spv")}
$hashes|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/provenance.json"
