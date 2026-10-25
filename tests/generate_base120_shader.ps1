# Isolated bounded log-mean and native-style10^mean, no public ABI extension.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/power"
$mean=[IO.File]::ReadAllText("$PSScriptRoot/mean120.comp")
$begin=$mean.IndexOf('void main(){');if($begin -lt 0){throw 'Missing mean main'}
$s=$mean.Substring(0,$begin)+[IO.File]::ReadAllText("$OutputDirectory/power/power_helpers.glsl")+"`n"
$main=$mean.Substring($begin)
$start=$main.IndexOf('    dst.words[group*121u]=mean;');if($start -lt 0){throw 'Missing mean output'}
$main=$main.Substring(0,$start)+@'
    dst.words[group*2u]=mean;
    dst.words[group*2u+1u]=floatBitsToUint(nativePower(10.0,uintBitsToFloat(mean)));
}
'@
[IO.File]::WriteAllText("$OutputDirectory/base120.comp",$s+$main,[Text.UTF8Encoding]::new($false))
