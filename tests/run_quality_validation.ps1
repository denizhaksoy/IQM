param(
    [Parameter(Mandatory=$true)][string]$ReferenceShader,
    [Parameter(Mandatory=$true)][string]$MathShaderDirectory,
    [string]$Octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',
    [switch]$HDR,
    [switch]$Stress,
    [string]$MapShader='',
    [string]$BaseFFTShader='',
    [ValidateSet(32,64)][int]$InputSize=64,
    [string]$GPUChecker='',
    [string]$CaseName='',
    [ValidateSet(30,60)][int]$PixelsPerDegree=30,
    [ValidateSet('24','60','60.001')][string]$ObserverAge='24',
    [string]$SpectralSamples='',
    [string]$SpectralDataDirectory='',
    [string]$LuminanceReference='',
    [string]$LuminanceTest=''
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location $root
$env:IQM_PREPROCESSING='fft'
$env:IQM_FFT_SIZE_FIXTURE='';$env:IQM_SPYR_DIMENSION_PROBE=''
$env:IQM_SPYR_SIZE=[string]$InputSize
$env:IQM_LUMINANCE_REFERENCE='';$env:IQM_LUMINANCE_TEST=''
$useLuminanceFiles=[bool]$LuminanceReference
if($useLuminanceFiles -ne [bool]$LuminanceTest){throw 'Both luminance file paths are required'}
if($useLuminanceFiles){
    $env:IQM_LUMINANCE_REFERENCE=(Resolve-Path -LiteralPath $LuminanceReference).Path
    $env:IQM_LUMINANCE_TEST=(Resolve-Path -LiteralPath $LuminanceTest).Path
}
$env:IQM_PIX_PER_DEGREE=[string]$PixelsPerDegree
$env:IQM_OBSERVER_AGE=$ObserverAge
$env:IQM_SPECTRAL_SAMPLES=''
$env:IQM_SPECTRAL_DATA=''
if($SpectralSamples -and $SpectralDataDirectory){throw 'Choose raw or resampled spectral inputs'}
if($SpectralDataDirectory){$env:IQM_SPECTRAL_DATA=(Resolve-Path -LiteralPath $SpectralDataDirectory).Path}
elseif($SpectralSamples){$env:IQM_SPECTRAL_SAMPLES=(Resolve-Path -LiteralPath $SpectralSamples).Path}
elseif($ObserverAge -ne '24'){throw 'Nondefault age requires pinned -SpectralSamples'}
$env:IQM_FFT_BACKEND='reference'
$env:IQM_FFT_AXIS_ORDER='column'
$env:IQM_FFT_TWIDDLES='native'
$env:IQM_FFT_REFERENCE_SHADER=(Resolve-Path -LiteralPath $ReferenceShader).Path
if($InputSize -eq 32){$env:IQM_FFT_REFERENCE_SHADER64=$env:IQM_FFT_REFERENCE_SHADER}
$env:IQM_FFT_MATH_SHADER_DIR=(Resolve-Path -LiteralPath $MathShaderDirectory).Path
$env:IQM_MAP_BACKEND='cpu'; $env:IQM_MAP_SHADER=''
$env:IQM_BASE_FFT_SHADER=''
if($MapShader) { $env:IQM_MAP_BACKEND='vulkan'; $env:IQM_MAP_SHADER=(Resolve-Path -LiteralPath $MapShader).Path }
if($BaseFFTShader) {
    if(!$MapShader) { throw '-BaseFFTShader requires -MapShader' }
    $env:IQM_BASE_FFT_SHADER=(Resolve-Path -LiteralPath $BaseFFTShader).Path
}
$prefix=if($HDR){'quality_hdr_suite'}else{'quality_suite'}
if($MapShader) { $prefix=if($HDR){'quality_hdr_gpu_maps_suite'}else{'quality_gpu_maps_suite'} }
if($BaseFFTShader) { $prefix=if($HDR){'quality_hdr_gpu_base_fft_suite'}else{'quality_gpu_base_fft_suite'} }
if($Stress) { $prefix=$prefix.Replace('quality_','quality_stress_') }
if($useLuminanceFiles){$prefix+='_file'}
if($InputSize -eq 32){$prefix+='_32'}
if($PixelsPerDegree -ne 30){$prefix+="_ppd$PixelsPerDegree"}
if($ObserverAge -ne '24'){$prefix+='_age'+$ObserverAge.Replace('.','p')}
if($CaseName){$prefix+='_selected'}
$batch=Join-Path $root ('validation/'+$prefix+'_'+[DateTime]::UtcNow.Ticks)
New-Item -ItemType Directory -Path $batch | Out-Null
Set-Content (Join-Path $root ('validation/'+$prefix+'_latest_path.txt')) $batch
$hashFiles=@((Join-Path $root 'build-vs2026/bin/Release/IQM.exe'),$env:IQM_FFT_REFERENCE_SHADER)
if($useLuminanceFiles){$hashFiles+=$env:IQM_LUMINANCE_REFERENCE;$hashFiles+=$env:IQM_LUMINANCE_TEST}
if($SpectralSamples){$hashFiles+=$env:IQM_SPECTRAL_SAMPLES}
if($SpectralDataDirectory){foreach($name in @('log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt','d65.csv')){$hashFiles+=Join-Path $env:IQM_SPECTRAL_DATA $name}}
if($MapShader) {
    if(!$GPUChecker){$GPUChecker=Join-Path $root 'validation/fft_publish_build/Release/IQM-map-gpu-test.exe'}
    $GPUChecker=(Resolve-Path -LiteralPath $GPUChecker).Path
    $hashFiles+=$env:IQM_MAP_SHADER
    $hashFiles+=$GPUChecker
}
if($BaseFFTShader) { $hashFiles+=$env:IQM_BASE_FFT_SHADER }
foreach($file in @('prepare.spv','finish.spv','multiply.spv')) { $hashFiles+=Join-Path $env:IQM_FFT_MATH_SHADER_DIR $file }
$hashes=foreach($file in $hashFiles) { Get-FileHash -LiteralPath $file -Algorithm SHA256 | Select-Object Path,Hash }
$hashes | ConvertTo-Json | Set-Content (Join-Path $batch 'binary_hashes.json')
$cases=@(
    @('asymmetric','localized'),@('asymmetric','stronger'),
    @('asymmetric','brighter'),@('diagonal','brighter'),@('impulse','brighter'),
    @('asymmetric','identical')
)
if($HDR) {
    $cases=@(@('hdr_ramp','localized'),@('hdr_edges','localized'),@('hdr_ramp','brighter'),@('hdr_edges','stronger'))
}
if($Stress) {
    $cases=@(@('sinusoidal_x','phase_shift'),@('sinusoidal_y','phase_shift'),@('asymmetric','localized_stronger'),
        @('asymmetric','threshold_below'),@('asymmetric','threshold_nominal'),@('asymmetric','threshold_above'))
    if($HDR) { $cases=@(@('hdr_edges','phase_shift'),@('hdr_ramp','localized_stronger')) }
}
if($useLuminanceFiles){$cases=@(,@('file','input'))}
$passedCases=0
foreach($case in $cases) {
    $pattern=$case[0]; $transform=$case[1]; $name="${pattern}_${transform}"
    if($CaseName -and $CaseName -ne $name){continue}
    $env:IQM_SPYR_PATTERN=$pattern; $env:IQM_SPYR_TEST_TRANSFORM=$transform
    $runLog=Join-Path $batch "${name}_run.log"
    $ErrorActionPreference='Continue'
    & (Join-Path $root 'build-vs2026/bin/Release/IQM.exe') --method HDRVDP3 --enable-validation > $runLog 2>&1
    $runExit=$LASTEXITCODE; $ErrorActionPreference='Stop'
    if($runExit -ne 0) { throw "IQM failed: $runLog" }
    if(Select-String -Path $runLog -Pattern 'ERROR in full pyramid|Validation Error') { throw "IQM validation failed: $runLog" }
    $line=Select-String -Path $runLog -Pattern 'SPYR validation directory:' | Select-Object -Last 1
    if(!$line) { throw "Missing export path: $runLog" }
    $export=(Resolve-Path ($line.Line.Split(':',2)[1].Trim())).Path
    if(!(Test-Path (Join-Path $export 'quality.txt'))) { throw "Incomplete export: $export" }
    if((Get-Content (Join-Path $export 'map_backend.txt')).Trim() -ne $env:IQM_MAP_BACKEND) { throw "Unexpected map backend: $export" }
    $expectedBase=if($BaseFFTShader){'vulkan_fft'}else{'cpu_dft'}
    if((Get-Content (Join-Path $export 'base_frequency_backend.txt')).Trim() -ne $expectedBase) { throw "Unexpected base FFT backend: $export" }
    $expectedSelection=if($BaseFFTShader){'vulkan'}else{'cpu'}
    if((Get-Content (Join-Path $export 'base_selection_backend.txt')).Trim() -ne $expectedSelection) { throw "Unexpected base selection backend: $export" }
    if((Get-Content (Join-Path $export 'base_csf_parameters_backend.txt')).Trim() -ne $env:IQM_MAP_BACKEND) { throw "Unexpected base CSF parameter backend: $export" }
    if((Get-Content (Join-Path $export 'comparison_mask_resize_backend.txt')).Trim() -ne $env:IQM_MAP_BACKEND) { throw "Unexpected comparison mask resize backend: $export" }
    Add-Content (Join-Path $batch 'exports.txt') "$name`t$export"
    if($useLuminanceFiles){
        $ErrorActionPreference='Continue'
        & $Octave --quiet --no-gui tests/validate_file_luminance.m $export $env:IQM_LUMINANCE_REFERENCE $env:IQM_LUMINANCE_TEST *> (Join-Path $batch ($name+'_input.log'))
        $inputExit=$LASTEXITCODE;$ErrorActionPreference='Stop'
        if($inputExit -ne 0){throw 'PFM/export input comparison failed'}
    }
    $checks=@(
        @('preprocessing','tests/validate_fft_preprocessing.m','--strict'),
        @('csf','tests/validate_csf_maps.m'),
        @('masking','tests/validate_masking_maps.m'),
        @('difference','tests/validate_difference_maps.m'),
        @('quality','tests/validate_quality_scores.m','--official')
    )
    foreach($check in $checks) {
        $log=Join-Path $batch ($name+'_'+$check[0]+'.log')
        $extra=@(); if($check.Count -gt 2) { $extra=@($check[2]) }
        # Octave may emit harmless shutdown warnings on stderr with exit0.
        $priorPreference=$ErrorActionPreference; $ErrorActionPreference='Continue'
        & $Octave --quiet --no-gui $check[1] $export @extra > $log 2>&1
        $checkExit=$LASTEXITCODE; $ErrorActionPreference=$priorPreference
        if($checkExit -ne 0) { Get-Content $log -Tail 15; throw "Check failed: $log" }
        if($check[0] -eq 'preprocessing') {
            $nativeLog=Join-Path $batch ($name+'_jnd_native.log')
            & (Join-Path $root 'build-vs2026/bin/Release/IQM-jnd-single-test.exe') $export > $nativeLog 2>&1
            if($LASTEXITCODE -ne 0) { Get-Content $nativeLog -Tail 15; throw "Native JND check failed: $nativeLog" }
        }
    }
    if($MapShader) {
        $log=Join-Path $batch ($name+'_gpu_intermediates.log')
        $ErrorActionPreference='Continue'
        & $GPUChecker $env:IQM_MAP_SHADER $export --maps > $log 2>&1
        $checkExit=$LASTEXITCODE; $ErrorActionPreference='Stop'
        if($checkExit -ne 0) { Get-Content $log -Tail 15; throw "GPU intermediate check failed: $log" }
    }
    Write-Output "$name PASS $export"
    ++$passedCases
    Select-String -Path (Join-Path $batch ($name+'_quality.log')) -Pattern 'OFFICIAL Q' | ForEach-Object {$_.Line}
}
if(!$passedCases){throw 'No quality cases selected'}
Set-Content (Join-Path $batch 'PASS.txt') "All $passedCases cases passed strict preprocessing, CSF, masking, D and official Q/Q_JOD comparisons. Vulkan maps additionally require independent GPU intermediates including all13 JND stages. HDR=$HDR maps=$env:IQM_MAP_BACKEND"
Write-Output "Suite PASS: $batch"
