param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin=$env:VULKAN_SDK+'/Bin')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
$tests=Split-Path $PSScriptRoot -Parent
& "$tests/generate_fft12_double_shader.ps1" -OutputDirectory "$OutputDirectory/source"
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$OutputDirectory/source/fft12_double.comp" -o "$OutputDirectory/default.spv"
if($LASTEXITCODE -ne 0){throw 'Compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$s=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if($s -notmatch 'OpExecutionMode %main LocalSize 16 1 1'){throw 'Unexpected entrypoint'}
$s=$s.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability RoundingModeRTE`n               OpCapability SignedZeroInfNanPreserve")
$s=$s.Replace('OpExecutionMode %main LocalSize 16 1 1',"OpExecutionMode %main LocalSize 16 1 1`n               OpExecutionMode %main RoundingModeRTE 64`n               OpExecutionMode %main SignedZeroInfNanPreserve 64")
[IO.File]::WriteAllText("$OutputDirectory/fft12_double.spvasm",$s,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/fft12_double.spvasm" -o "$OutputDirectory/fft12_double.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/fft12_double.spv"
if($LASTEXITCODE -ne 0){throw 'Validation failed'}
Copy-Item "$tests/fftw_reference/COPYING" "$OutputDirectory/COPYING"
Get-FileHash "$OutputDirectory/fft12_double.spv" | Select-Object Hash | ConvertTo-Json | Set-Content "$OutputDirectory/sha256.json"
