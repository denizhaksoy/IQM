param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$RawData,[switch]$GPUProjection,[switch]$GPUMasking,[switch]$GPUNeighbor,[switch]$GPUPyramid,[switch]$GPUComparison,[switch]$GPUJND,[switch]$GPUCSF,[switch]$GPUQuality,[switch]$GPULogarithm)
$ErrorActionPreference='Stop'
if($GPULogarithm){$GPUQuality=$true}
if($GPUQuality){$GPUCSF=$true}
if($GPUCSF){$GPUJND=$true}
if($GPUJND){$GPUComparison=$true}
if($GPUComparison){$GPUPyramid=$true}
if($GPUPyramid){$GPUNeighbor=$true}
if($GPUNeighbor){$GPUMasking=$true}
if($GPUMasking){$GPUProjection=$true}
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory "$OutputDirectory/build","$OutputDirectory/runtime/shaders","$OutputDirectory/runtime/model"|Out-Null
& "$PSScriptRoot/compile_forward192_double.ps1" -OutputDirectory "$OutputDirectory/build/forward"
& "$PSScriptRoot/compile_inverse192_double.ps1" -OutputDirectory "$OutputDirectory/build/inverse"
& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/multiply"
& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/crop" -ShaderSource "$PSScriptRoot/../double192_crop.comp"
& "$PSScriptRoot/compile_fft12_double.ps1" -OutputDirectory "$OutputDirectory/build/base"
$assets=[ordered]@{
 'forward-first'='build/forward/first/real192_double.spv';'forward-second'='build/forward/second/complex192_double.spv';'multiply'='build/multiply/multiply.spv';'inverse-first'='build/inverse/first/inverse192_double_axis.spv';'inverse-second'='build/inverse/second/complex192_double_inverse.spv';'crop'='build/crop/multiply.spv';'base12'='build/base/fft12_double.spv'
}
if($GPUProjection){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/projection" -ShaderSource "$PSScriptRoot/../double96_projection.comp";$assets["projection"]="build/projection/multiply.spv"}
if($GPUMasking){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/mutual" -ShaderSource "$PSScriptRoot/../double96_mutual.comp";$assets["mutual"]="build/mutual/multiply.spv"}
if($GPUNeighbor){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/neighbor" -ShaderSource "$PSScriptRoot/../double96_neighbor.comp";$assets["neighbor"]="build/neighbor/multiply.spv"}
if($GPUPyramid){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/pyramid" -ShaderSource "$PSScriptRoot/../double96_pyramid.comp";$assets["pyramid"]="build/pyramid/multiply.spv"}
if($GPUComparison){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/comparison" -ShaderSource "$PSScriptRoot/../double96_comparison.comp";$assets["comparison"]="build/comparison/multiply.spv"}
if($GPUJND){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/jnd" -ShaderSource "$PSScriptRoot/../double96_jnd.comp";$assets["jnd"]="build/jnd/multiply.spv"}
if($GPUCSF){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/csf" -ShaderSource "$PSScriptRoot/../double96_csf.comp";$assets["csf"]="build/csf/multiply.spv"}
if($GPUQuality){& "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/quality" -ShaderSource "$PSScriptRoot/../double96_quality.comp";$assets["quality"]="build/quality/multiply.spv"}
if($GPULogarithm){
 & node "$PSScriptRoot/../hdrvdp3_evaluation/generate_winlibm_log_candidate.cjs" "$PSScriptRoot/../hdrvdp3_models/winlibm" "$OutputDirectory/build/logarithm.comp" fma
 if($LASTEXITCODE -ne 0){throw 'Logarithm generation failed'}
 & "$PSScriptRoot/compile_double192_multiply.ps1" -OutputDirectory "$OutputDirectory/build/logarithm" -ShaderSource "$OutputDirectory/build/logarithm.comp"
 $assets["logarithm"]="build/logarithm/multiply.spv"
 Copy-Item "$PSScriptRoot/../hdrvdp3_models/winlibm/log.asm" "$OutputDirectory/runtime/COPYING-win-libm.asm"
}
$version=if($GPULogarithm){10}elseif($GPUQuality){9}elseif($GPUCSF){8}elseif($GPUJND){7}elseif($GPUComparison){6}elseif($GPUPyramid){5}elseif($GPUNeighbor){4}elseif($GPUMasking){3}elseif($GPUProjection){2}else{1}
$lines=@("IQM_HDRVDP3_DOUBLE96 $version srgb-display-quality-30ppd-age24-sp0")
foreach($key in $assets.Keys){$relative="shaders/$key.spv";Copy-Item "$OutputDirectory/$($assets[$key])" "$OutputDirectory/runtime/$relative";$hash=(Get-FileHash "$OutputDirectory/runtime/$relative").Hash.ToLowerInvariant();$lines+="$key `"$relative`" $hash"}
foreach($name in @('ciexyz31.csv','log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt','emission_spectra_led-lcd-srgb.csv','emission_spectra_led-lcd-wcg.csv','emission_spectra_oled.csv')){Copy-Item "$RawData/$name" "$OutputDirectory/runtime/model/$name";$hash=(Get-FileHash "$OutputDirectory/runtime/model/$name").Hash.ToLowerInvariant();$lines+="$name `"model/$name`" $hash"}
[IO.File]::WriteAllLines("$OutputDirectory/runtime/double96.runtime",$lines,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/../fftw_reference/COPYING" "$OutputDirectory/runtime/COPYING-FFTW"
"Fixed96x96/30ppd/age24/quality/defaultsp0;Double FFT GPU;GPU optical spectral projection=$GPUProjection;GPU mutual masking=$GPUMasking;GPU neighbor masking=$GPUNeighbor;GPU pyramid=$GPUPyramid;GPU comparison=$GPUComparison;GPU JND linear=$GPUJND (CPU reduction/exp/pupil/slopes;log query backend listed below);GPU CSF maps=$GPUCSF (CPU model/base nonlinear scalar);GPU quality arithmetic/pooling=$GPUQuality (CPU pow/psychometric/final scalar);GPU JND logarithms=$GPULogarithm (CPU reduction/exp/pupil/slopes retained);remaining visual preprocessing CPU. Requires Float64/RTE64/SignedZeroInfNan64;no generic denorm/device parity." | Set-Content "$OutputDirectory/runtime/PROFILE.txt"
Get-ChildItem "$OutputDirectory/runtime" -Recurse -File|Get-FileHash|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/runtime_hashes.json"
