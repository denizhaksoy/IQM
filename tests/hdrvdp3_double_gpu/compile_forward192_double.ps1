param([Parameter(Mandatory=$true)][string]$OutputDirectory,
 [string]$VulkanBin=$env:VULKAN_SDK+'/Bin',
 [string]$Gcc='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/gcc.exe')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
$tests=Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/compile_real192_double_axis.ps1" -OutputDirectory "$OutputDirectory/first" -VulkanBin $VulkanBin -Gcc $Gcc
$env:PATH="$(Split-Path $Gcc);$env:PATH"
& "$OutputDirectory/first/prepare-twiddles.exe" "$OutputDirectory/complex_twiddles.f64" complex
if($LASTEXITCODE -ne 0){throw 'Complex twiddle preparation failed'}
& "$tests/generate_complex192_double_shader.ps1" -OutputDirectory "$OutputDirectory/second" -TwiddleFile "$OutputDirectory/complex_twiddles.f64" -T1Graph 0 -T3Graph 0
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$OutputDirectory/second/complex192_double.comp" -o "$OutputDirectory/second/default.spv"
if($LASTEXITCODE -ne 0){throw 'Complex shader compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/second/default.spv" -o "$OutputDirectory/second/default.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$s=[IO.File]::ReadAllText("$OutputDirectory/second/default.spvasm")
if($s -notmatch 'OpExecutionMode %main LocalSize 16 1 1'){throw 'Unexpected complex entrypoint'}
$s=$s.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability RoundingModeRTE`n               OpCapability SignedZeroInfNanPreserve")
$s=$s.Replace('OpExecutionMode %main LocalSize 16 1 1',"OpExecutionMode %main LocalSize 16 1 1`n               OpExecutionMode %main RoundingModeRTE 64`n               OpExecutionMode %main SignedZeroInfNanPreserve 64")
[IO.File]::WriteAllText("$OutputDirectory/second/complex192_double.spvasm",$s,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/second/complex192_double.spvasm" -o "$OutputDirectory/second/complex192_double.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/second/complex192_double.spv"
if($LASTEXITCODE -ne 0){throw 'Validation failed'}
Copy-Item "$tests/fftw_reference/COPYING" "$OutputDirectory/COPYING"
Get-FileHash "$OutputDirectory/first/real192_double.spv","$OutputDirectory/second/complex192_double.spv","$OutputDirectory/complex_twiddles.f64" | Select-Object Path,Hash | ConvertTo-Json | Set-Content "$OutputDirectory/sha256.json"
