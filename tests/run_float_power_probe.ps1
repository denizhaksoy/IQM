param(
    [Parameter(Mandatory=$true)][string]$MathShaderDirectory,
    [string]$Octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe',
    [string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root ('validation/power_probe_' + [DateTime]::UtcNow.Ticks)
New-Item -ItemType Directory -Path $output | Out-Null
$fixture=Join-Path $output 'fixture'
# Octave may print a shutdown warning to stderr despite exit0.
$ErrorActionPreference='Continue'
& $Octave --quiet (Join-Path $PSScriptRoot 'generate_float_power_reference.m') $fixture *> (Join-Path $output 'reference.log')
$referenceExit=$LASTEXITCODE
$ErrorActionPreference='Stop'
if($referenceExit -ne 0) { throw "Reference failed: $referenceExit" }
$math=Get-Content (Join-Path $MathShaderDirectory 'math.comp') -Raw
$needle='dst.value[index]=vec2(accurateLog(v.x),accurateExp(v.y));'
if(-not $math.Contains($needle)) { throw 'Unexpected reference math source' }
$replacement='precise float t=v.y*accurateLog(v.x); float result=(v.x==0.0)?0.0:accurateExp(t); dst.value[index]=vec2(result,result);'
$source=Join-Path $output 'source_power.comp'
Set-Content $source $math.Replace($needle,$replacement)
$results=@()
foreach($kind in @('native','source')) {
    $shaderSource=$source
    if($kind -eq 'native') { $shaderSource=Join-Path $PSScriptRoot 'float_power_probe.comp' }
    $spv=Join-Path $output ($kind+'.spv')
    & (Join-Path $VulkanBin 'glslc.exe') -fshader-stage=compute --target-env=vulkan1.2 $shaderSource -o $spv
    if($LASTEXITCODE -ne 0) { throw 'Shader compilation failed' }
    & (Join-Path $VulkanBin 'spirv-val.exe') --target-env vulkan1.2 $spv
    if($LASTEXITCODE -ne 0) { throw 'SPIR-V validation failed' }
    $ErrorActionPreference='Continue'
    & (Join-Path $root 'validation/fft_publish_build/Release/IQM-fft-codelet-test.exe') $fixture $spv --float-math *> (Join-Path $output ($kind+'.log'))
    $probeExit=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($probeExit -notin @(0,1)) { throw "Probe runtime failure: $probeExit" }
    $results+=@{variant=$kind; exit=$probeExit; shader_sha256=(Get-FileHash $spv).Hash}
}
$results | ConvertTo-Json | Set-Content (Join-Path $output 'results.json')
Set-Content (Join-Path $root 'validation/power_probe_latest_path.txt') $output
Write-Output $output
# Exit0 means the experiment completed. Inspect results.json for parity results.
