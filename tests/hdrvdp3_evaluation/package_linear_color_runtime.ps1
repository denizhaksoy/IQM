param([Parameter(Mandatory=$true)][string]$NativeRuntime,[Parameter(Mandatory=$true)][string]$Conversion,[Parameter(Mandatory=$true)][ValidateSet('rgb-bt.2020','xyz')][string]$Encoding,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';if(Test-Path $OutputDirectory){throw 'Fresh linear-color package required'}
if((Get-Content "$NativeRuntime/rgb.runtime" -First 1) -ne 'IQM_HDRVDP3_RGB 1 rgb-native-30ppd-age24-sp0-square96'){throw 'NativeRGB package required'}
if(!(Test-Path $Conversion -PathType Leaf)){throw 'Missing conversion shader'}
New-Item -ItemType Directory "$OutputDirectory/assets" -Force|Out-Null
Copy-Item -LiteralPath $NativeRuntime -Destination "$OutputDirectory/native" -Recurse
Copy-Item -LiteralPath $Conversion -Destination "$OutputDirectory/assets/conversion.spv"
$lines=@("IQM_HDRVDP3_LINEAR 1 $Encoding-30ppd-age24-sp0-square96")
$lines+='native "native/rgb.runtime" '+(Get-FileHash "$OutputDirectory/native/rgb.runtime").Hash.ToLowerInvariant()
$lines+='conversion "assets/conversion.spv" '+(Get-FileHash "$OutputDirectory/assets/conversion.spv").Hash.ToLowerInvariant()
[IO.File]::WriteAllLines("$OutputDirectory/linear.runtime",$lines,[Text.UTF8Encoding]::new($false))
if(Test-Path "$PSScriptRoot/../LINEAR_COLOR_VALIDATION.md"){
 Copy-Item -LiteralPath "$PSScriptRoot/../LINEAR_COLOR_VALIDATION.md" -Destination "$OutputDirectory/VALIDATION.md"
}
