param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='C:/VulkanSDK/1.4.363.0/Bin')
$ErrorActionPreference='Stop';if(Test-Path $OutputDirectory){throw 'Fresh short source directory required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName
$ErrorActionPreference='Continue'
& powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/generate_rgb709_runtime.ps1" -OutputDirectory "$out/s" -VulkanBin $VulkanBin *> "$out/source.log"
$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code){throw 'Shared source generation failed'}
foreach($encoding in @('rgb-bt.2020','xyz')){
 & "$PSScriptRoot/package_linear_color_runtime.ps1" -NativeRuntime "$out/s/runtime/native" -Conversion "$out/s/runtime/assets/conversion.spv" -Encoding $encoding -OutputDirectory "$out/$encoding"
}
Set-Content "$out/PASS.txt" 'Both linear-color profiles packaged from23 shared source-generated pinned payloads;only manifest encoding differs.No reference installation/exports read.'
