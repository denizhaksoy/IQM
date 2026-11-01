param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin=$env:VULKAN_SDK+'/Bin',[string]$ShaderSource=$PSScriptRoot+'/../double192_filter_multiply.comp')
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 $ShaderSource -o "$OutputDirectory/default.spv"
if($LASTEXITCODE -ne 0){throw 'Compile failed'}
& "$VulkanBin/spirv-dis.exe" "$OutputDirectory/default.spv" -o "$OutputDirectory/default.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$s=[IO.File]::ReadAllText("$OutputDirectory/default.spvasm")
if(!$s.Contains('OpExecutionMode %main LocalSize 64 1 1')){throw 'Unexpected entrypoint'}
$s=$s.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability RoundingModeRTE`n               OpCapability SignedZeroInfNanPreserve")
$s=$s.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n               OpExecutionMode %main RoundingModeRTE 64`n               OpExecutionMode %main SignedZeroInfNanPreserve 64")
[IO.File]::WriteAllText("$OutputDirectory/multiply.spvasm",$s,[Text.UTF8Encoding]::new($false))
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$OutputDirectory/multiply.spvasm" -o "$OutputDirectory/multiply.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/multiply.spv"
if($LASTEXITCODE -ne 0){throw 'Validation failed'}
