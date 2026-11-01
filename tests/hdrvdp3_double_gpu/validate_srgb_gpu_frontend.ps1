param(
 [Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$CpuRoot,
 [Parameter(Mandatory=$true)][string]$EncodedRoot,
 [Parameter(Mandatory=$true)][string]$ChainRoot,
 [Parameter(Mandatory=$true)][string]$BaseRoot,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [string]$RawData='C:/HDR-VDP-3_Ref/data')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
$cases=@(@('stress',"$CpuRoot/native","$EncodedRoot/native_stress"),@('ramps',"$CpuRoot/native_ramps","$EncodedRoot/native"),@('identity',"$CpuRoot/native_identity_v2","$CpuRoot/input_identity"))
@{cpu=[string]$CpuRoot;encoded=[string]$EncodedRoot;chain=[string]$ChainRoot;base=[string]$BaseRoot}|ConvertTo-Json|Set-Content "$OutputDirectory/lineage.json"
foreach($case in $cases){
 $tag=$case[0]
 & $Executable $case[1] "$OutputDirectory/allfft_$tag" unaligned $RawData $case[2] "$ChainRoot/dependency_shaders/forward-first.spv" "$ChainRoot/dependency_shaders/forward-second.spv" "$ChainRoot/multiply_pack/multiply.spv" "$ChainRoot/dependency_shaders/inverse-first.spv" "$ChainRoot/dependency_shaders/inverse-second.spv" "$ChainRoot/crop_pack/multiply.spv" "$BaseRoot/repro/fft12_double.spv" *> "$OutputDirectory/allfft_$tag.log"
 if($LASTEXITCODE -ne 0){throw "GPU frontend failed: $tag;see log"}
 Get-Content "$OutputDirectory/allfft_$tag/summary.txt"
}
& node "$PSScriptRoot/../hdrvdp3_evaluation/audit_srgb_gpu_frontend.cjs" $OutputDirectory
if($LASTEXITCODE -ne 0){throw 'Final audit failed'}
