# Variable-count log mean and native-style power10; isolated alternate ABI.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/power"
$mean=[IO.File]::ReadAllText("$PSScriptRoot/mean_general.comp")
$begin=$mean.IndexOf('void main(){');if($begin -lt 0){throw 'Missing general mean main'}
$main=$mean.Substring($begin)
$start=$main.IndexOf('    uint outputOffset=');if($start -lt 0){throw 'Missing output anchor'}
$main=$main.Substring(0,$start)+@'
    dst.words[group*2u]=mean;
    dst.words[group*2u+1u]=floatBitsToUint(nativePower(10.0,uintBitsToFloat(mean)));
}
'@
$source=$mean.Substring(0,$begin)+[IO.File]::ReadAllText("$OutputDirectory/power/power_helpers.glsl")+"`n"+$main
[IO.File]::WriteAllText("$OutputDirectory/base_general.comp",$source,[Text.UTF8Encoding]::new($false))
