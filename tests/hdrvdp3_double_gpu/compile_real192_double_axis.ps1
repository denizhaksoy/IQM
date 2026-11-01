param([Parameter(Mandatory=$true)][string]$OutputDirectory,
 [string]$VulkanBin=$env:VULKAN_SDK+'/Bin',
 [string]$Gcc='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/gcc.exe')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
$tests=Split-Path $PSScriptRoot -Parent
$env:PATH="$(Split-Path $Gcc);$env:PATH"
& $Gcc -O2 -ffp-contract=off "$tests/prepare_double192_twiddles.c" -o "$OutputDirectory/prepare-twiddles.exe"
if($LASTEXITCODE -ne 0){throw 'Twiddle helper build failed'}
& "$OutputDirectory/prepare-twiddles.exe" "$OutputDirectory/twiddles.f64" direct
if($LASTEXITCODE -ne 0){throw 'Twiddle preparation failed'}
& "$tests/generate_fft192_double_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/source" -TwiddleFile "$OutputDirectory/twiddles.f64"
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$OutputDirectory/source/forward192_double_axis.comp" -o "$OutputDirectory/default.spv"
if($LASTEXITCODE -ne 0){throw 'Shader compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$s=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if($s -notmatch 'OpExecutionMode %main LocalSize 32 1 1'){throw 'Unexpected entrypoint'}
$s=$s.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability RoundingModeRTE`n               OpCapability SignedZeroInfNanPreserve")
$s=$s.Replace('OpExecutionMode %main LocalSize 32 1 1',"OpExecutionMode %main LocalSize 32 1 1`n               OpExecutionMode %main RoundingModeRTE 64`n               OpExecutionMode %main SignedZeroInfNanPreserve 64")
[IO.File]::WriteAllText("$OutputDirectory/real192_double.spvasm",$s,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/real192_double.spvasm" -o "$OutputDirectory/real192_double.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/real192_double.spv"
if($LASTEXITCODE -ne 0){throw 'Validation failed'}
Copy-Item "$tests/fftw_reference/COPYING" "$OutputDirectory/COPYING"
Get-FileHash "$OutputDirectory/real192_double.spv","$OutputDirectory/twiddles.f64" | Select-Object Path,Hash | ConvertTo-Json | Set-Content "$OutputDirectory/sha256.json"
