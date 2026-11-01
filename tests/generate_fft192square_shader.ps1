# Isolated192x192 FFTW-plan diagnostic. Preserve all mixed runtime binaries.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet('forward','inverse')][string]$Direction='forward')
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
if($Direction -eq 'forward'){
    & "$PSScriptRoot/generate_fft160x192_forward_shader.ps1" -OutputDirectory "$OutputDirectory/mixed"
    & "$PSScriptRoot/generate_fft192_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/axis"
    $mixed=[IO.File]::ReadAllText("$OutputDirectory/mixed/forward160x192.comp")
    $axis=[IO.File]::ReadAllText("$OutputDirectory/axis/forward192_axis.comp")
    $begin=$mixed.IndexOf('void r2cf_8');$end=$mixed.IndexOf('vec2 BYTWJ1(')
    if($begin -lt 0 -or $end -le $begin){throw 'Unexpected forward graph anchors'}
    $s=$mixed.Substring(0,$begin)+$axis.Substring($axis.IndexOf('void r2cf_8')).Replace('void main()','void firstAxis()')+"`n"+$mixed.Substring($end)
    $s=$s.Replace('batch>=81u','batch>=97u').Replace('*81u+batch','*97u+batch')
    $s=$s.Replace('frequency*160u+batch','frequency*192u+batch').Replace('batch<80u','batch<96u')
    $s=$s.Replace('*160u+160u-batch','*192u+192u-batch')
}else{
    & "$PSScriptRoot/generate_fft160x192_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/mixed"
    & "$PSScriptRoot/generate_fft192_inverse_axis_shader.ps1" -OutputDirectory "$OutputDirectory/axis"
    $mixed=[IO.File]::ReadAllText("$OutputDirectory/mixed/inverse160x192.comp")
    $axis=[IO.File]::ReadAllText("$OutputDirectory/axis/inverse192_axis.comp")
    foreach($name in @('VADD','VSUB','VMUL','VFMAI','VFNMSI')){
        $axis=[regex]::Replace($axis,"(vec2 $name\([^\r\n]+?\)\s*\{)\s*return ([^;]+);\s*\}",'$1 precise vec2 result=$2; return result; }')
    }
    $begin=$mixed.IndexOf('vec2 BYTW1(')
    if($begin -lt 0){throw 'Unexpected inverse graph anchor'}
    $s=$axis.Substring(0,$axis.IndexOf('void main()'))+$axis.Substring($axis.IndexOf('void main()')).Replace('void main()','void firstAxis()')+"`n"+$mixed.Substring($begin)
    $begin=$s.IndexOf('uint divideRounded');$end=$s.LastIndexOf('void main()')
    $normalization=[IO.File]::ReadAllText("$PSScriptRoot/square192_normalization.comp")
    $s=$s.Substring(0,$begin)+$normalization.Substring($normalization.IndexOf('uint divideRounded'))+"`n"+$s.Substring($end)
    $s=$s.Replace('batch>=160u','batch>=192u').Replace('frequency*160u+batch','frequency*192u+batch')
}
[IO.File]::WriteAllText("$OutputDirectory/${Direction}192square.comp",$s,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
