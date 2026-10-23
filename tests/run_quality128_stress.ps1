param(
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [Parameter(Mandatory=$true)][string]$DiagnosticExecutable,
 [Parameter(Mandatory=$true)][string]$Assets,
 [Parameter(Mandatory=$true)][string]$SpectralDirectory,
 [Parameter(Mandatory=$true)][string]$SourceExport,
 [string]$Octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe'
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $repo
$exe=(Resolve-Path -LiteralPath $DiagnosticExecutable).Path
$assetsPath=(Resolve-Path -LiteralPath $Assets).Path
$spectral=(Resolve-Path -LiteralPath $SpectralDirectory).Path
$source=(Resolve-Path -LiteralPath $SourceExport).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh suite directory required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
function Run-Checked([string]$program,[string[]]$arguments,[string]$log){
 $ErrorActionPreference='Continue'
 & $program @arguments *> $log
 $code=$LASTEXITCODE
 $ErrorActionPreference='Stop'
 Get-Content -LiteralPath $log -Tail 3
 if($code){throw "Command failed($code):$program;see $log"}
 if(Select-String -LiteralPath $log -Pattern 'VUID-|Validation Error|\[ERROR\]'){throw "Vulkan diagnostic:$log"}
}
$files=@($exe,"$PSScriptRoot/generate_quality128_reference.m",$PSCommandPath)
$files+=@('validate_fft_preprocessing','validate_spyr_spatial','validate_csf_maps','validate_masking_maps','validate_difference_maps','validate_quality_scores')|ForEach-Object {"$PSScriptRoot/$_.m"}
$files+=@(Get-ChildItem -LiteralPath $assetsPath -Recurse -Filter '*.spv'|ForEach-Object {$_.FullName})
Get-FileHash -LiteralPath $files|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
foreach($pin in (Get-Content "$PSScriptRoot/hdrvdp3_evaluation/REFERENCE_SHADER_HASHES128.json" -Raw|ConvertFrom-Json)){
 if((Get-FileHash -LiteralPath "$assetsPath/$($pin.file)").Hash -ne $pin.sha256){throw "Runtime shader pin mismatch:$($pin.file)"}
}
Get-ChildItem -LiteralPath $spectral -Recurse -File|Get-FileHash|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/spectral_hashes.json"
$records=@()
foreach($case in @('impulse','competing_axes')){
 $native="$root/native_$case";$export="$root/$case"
 Run-Checked $Octave @('--quiet','--no-gui','tests/generate_quality128_reference.m',$source,$native,$case) "$root/native_$case.log"
 Get-FileHash -LiteralPath "$native/reference.pfm","$native/test.pfm"|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/${case}_input_hashes.json"
 $scores=((Get-Content "$native/scores.txt" -Raw).Trim() -split '\s+')
 Run-Checked $exe @("$native/reference.pfm","$native/test.pfm","$assetsPath/fft.spv","$assetsPath/math","$assetsPath/map.spv","$assetsPath/base.spv",$spectral,$export,$scores[0],$scores[1]) "$root/gpu_$case.log"
 foreach($check in @('validate_fft_preprocessing','validate_spyr_spatial','validate_csf_maps','validate_masking_maps','validate_difference_maps','validate_quality_scores')){
  $arguments=@('--quiet','--no-gui',"tests/$check.m",$export)
  if($check -eq 'validate_fft_preprocessing'){$arguments+='--strict'}
  if($check -eq 'validate_quality_scores'){$arguments+='--official'}
  Run-Checked $Octave $arguments "$root/${case}_$check.log"
 }
 $ranking=((Get-Content "$export/base_native_ranking.txt")[-1].Trim() -split '\s+')
 $culture=[Globalization.CultureInfo]::InvariantCulture
 $top=[float]::Parse($ranking[1],$culture);$next=[float]::Parse($ranking[2],$culture)
 $gap=[BitConverter]::ToInt32([BitConverter]::GetBytes($top),0)-[BitConverter]::ToInt32([BitConverter]::GetBytes($next),0)
 if($case -eq 'competing_axes' -and $gap -ne 1){throw 'Fixture no longer exercises a one-ULP residual near-tie'}
 $records+=[ordered]@{case=$case;native_checks=6;Q=$scores[0];Q_JOD=$scores[1];numeric_mismatches=0;maximum_index=[int]$ranking[0];top_distinct_gap_ULP=$gap}
 $records|ConvertTo-Json|Set-Content "$root/results.json"
}
[ordered]@{scope='128square,30ppd,age24,D65,Float32 luminance';cases=2;native_checks=12;fft='256square,column-major;inverse1/65536';padding='MTF symmetric,LA replicate';dc='residual mean removed;selection magnitude DC zero';contracts='FFT256,map14,base82';near_tie_claim='competing_axes top distinct residual magnitudes differ by one Float32 ULP;first-maximum index exact';signed_zero_bits_asserted=$false}|ConvertTo-Json|Set-Content "$root/COMPLETED.json"
Set-Content "$root/PASS.txt" 'Both cases passed all six exact native checks.'
