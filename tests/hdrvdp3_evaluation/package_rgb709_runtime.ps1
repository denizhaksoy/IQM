param([Parameter(Mandatory=$true)][string]$NativeRuntime,[Parameter(Mandatory=$true)][string]$Conversion,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh709 package required'}
if((Get-Content -LiteralPath "$NativeRuntime/rgb.runtime" -First 1) -ne 'IQM_HDRVDP3_RGB 1 rgb-native-30ppd-age24-sp0-square96'){throw '709 requires nativeRGB runtime'}
if(!(Test-Path -LiteralPath $Conversion -PathType Leaf)){throw 'Missing conversion shader'}
New-Item -ItemType Directory "$OutputDirectory/assets" -Force | Out-Null
Copy-Item -LiteralPath $NativeRuntime -Destination "$OutputDirectory/native" -Recurse
Copy-Item -LiteralPath $Conversion -Destination "$OutputDirectory/assets/conversion.spv"
$lines=@('IQM_HDRVDP3_709 1 rgb-bt.709-30ppd-age24-sp0-square96')
$lines+='native "native/rgb.runtime" '+(Get-FileHash "$OutputDirectory/native/rgb.runtime").Hash.ToLowerInvariant()
$lines+='conversion "assets/conversion.spv" '+(Get-FileHash "$OutputDirectory/assets/conversion.spv").Hash.ToLowerInvariant()
[IO.File]::WriteAllLines("$OutputDirectory/rgb709.runtime",$lines,[Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath "$PSScriptRoot/../XYZ709_CALIBRATION_VALIDATION.md" -Destination "$OutputDirectory/VALIDATION.md"
if(Test-Path -LiteralPath "$PSScriptRoot/../RGB709_CLI_VALIDATION.md"){
 Copy-Item -LiteralPath "$PSScriptRoot/../RGB709_CLI_VALIDATION.md" -Destination "$OutputDirectory/CLI_VALIDATION.md"
}
