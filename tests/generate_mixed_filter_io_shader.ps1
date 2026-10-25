param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/power"
$math=[IO.File]::ReadAllText("$OutputDirectory/power/power_helpers.glsl")
$multiply=[IO.File]::ReadAllText("$PSScriptRoot/float_multiply.comp")
$start=$multiply.IndexOf('uint roundedShift');$end=$multiply.IndexOf('void main()')
if($start -lt 0 -or $end -le $start){throw 'Missing exact product helper'}
$math+=$multiply.Substring($start,$end-$start)
$s=[IO.File]::ReadAllText("$PSScriptRoot/mixed_filter_io.comp")
if([regex]::Matches($s,'// MATH_HELPERS').Count -ne 1){throw 'Missing math marker'}
[IO.File]::WriteAllText("$OutputDirectory/mixed_filter_io.comp",$s.Replace('// MATH_HELPERS',$math))
