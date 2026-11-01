# Isolated160square graphs; preserve all accepted mixed/192square payloads.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet('forward','inverse')][string]$Direction='forward')
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
if($Direction-eq 'forward'){
 & "$PSScriptRoot/generate_fft192x160_forward_shader.ps1" -OutputDirectory "$OutputDirectory/mixed"
 & "$PSScriptRoot/generate_fft160_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/axis"
 $mixed=[IO.File]::ReadAllText("$OutputDirectory/mixed/forward192x160.comp")
 $axis=[IO.File]::ReadAllText("$OutputDirectory/axis/forward160_axis.comp")
 $begin=$mixed.IndexOf('void r2cf_8');$end=$mixed.IndexOf('void leaf8(')
 if($begin-lt 0-or $end-le $begin){throw 'Forward graph anchors changed'}
 $s=$mixed.Substring(0,$begin)+$axis.Substring($axis.IndexOf('void r2cf_8')).Replace('void main()','void firstAxis()')+"`n"+$mixed.Substring($end)
 $s=$s.Replace('batch>=97u','batch>=81u').Replace('*97u+batch','*81u+batch').Replace('f*192u+batch','f*160u+batch').Replace('batch<96u','batch<80u').Replace('*192u+192u-batch','*160u+160u-batch')
}else{
 & "$PSScriptRoot/generate_fft192x160_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/mixed"
 & "$PSScriptRoot/generate_fft160_inverse_axis_shader.ps1" -OutputDirectory "$OutputDirectory/axis"
 $mixed=[IO.File]::ReadAllText("$OutputDirectory/mixed/inverse192x160.comp")
 $axis=[IO.File]::ReadAllText("$OutputDirectory/axis/inverse160_axis.comp")
 $prefix=$axis.Substring(0,$axis.IndexOf('void main()'))
 foreach($name in @('VADD','VSUB','VMUL','VFMAI','VFNMSI')){
  $prefix=[regex]::Replace($prefix,"(vec2 $name\([^\r\n]+?\)\s*\{)\s*precise vec2 result=([^;]+);\s*return result;\s*\}",'$1 return $2; }')
 }
 $begin=$mixed.IndexOf('vec2 VFMS');$end=$mixed.IndexOf('void firstAxis()');$tail=$mixed.IndexOf('void leaf8(')
 if($begin-lt 0-or $end-le $begin-or $tail-le $end){throw 'Inverse graph anchors changed'}
 $s=$prefix+$mixed.Substring($begin,$end-$begin)+$axis.Substring($axis.IndexOf('void main()')).Replace('void main()','void firstAxis()')+"`n"+$mixed.Substring($tail)
 $begin=$s.IndexOf('uint divideRounded');$end=$s.LastIndexOf('void main()')
 $normalization=[IO.File]::ReadAllText("$PSScriptRoot/square160_normalization.comp")
 if($begin-lt 0-or $end-le $begin){throw 'Normalization anchors changed'}
 $s=$s.Substring(0,$begin)+$normalization.Substring($normalization.IndexOf('uint divideRounded'))+"`n"+$s.Substring($end)
 $s=$s.Replace('batch>=192u','batch>=160u').Replace('f*192u+batch','f*160u+batch')
}
[IO.File]::WriteAllText("$OutputDirectory/${Direction}160square.comp",$s,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
