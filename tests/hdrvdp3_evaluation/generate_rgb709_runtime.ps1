param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin')
$ErrorActionPreference='Stop';if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh709 source build required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName
$ErrorActionPreference='Continue'
& powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/generate_rgb_runtime.ps1" -OutputDirectory "$out/rgb" -VulkanBin $VulkanBin *> "$out/rgb.log"
$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code){throw 'NativeRGB generation failed'}
$ErrorActionPreference='Continue'
& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$PSScriptRoot/../../shaders/hdrvdp3/hdrvdp3_rgb709_projection.comp" -o "$out/conversion.spv" *> "$out/conversion.log"
$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code){throw '709 compile failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$out/conversion.spv";if($LASTEXITCODE){throw '709 SPIR-V invalid'}
& "$PSScriptRoot/package_rgb709_runtime.ps1" -NativeRuntime "$out/rgb/runtime" -Conversion "$out/conversion.spv" -OutputDirectory "$out/runtime"
$pin=Get-Content "$PSScriptRoot/REFERENCE_RGB709_HASHES.json" -Raw|ConvertFrom-Json
if((Get-FileHash "$out/runtime/$($pin.file)").Hash -ne $pin.sha256){throw '709 conversion pin differs'}
Set-Content "$out/PASS.txt" '23 source-generated shader/model payloads match pins:22 nativeRGB +1 conversion.No native installation/reference exports required.'
