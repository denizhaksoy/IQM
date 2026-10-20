# Source-backed, fixed-graph numerical experiment. Does not read Windows DLLs.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$reference=Join-Path $PSScriptRoot 'math_reference'
$original=Join-Path $reference 'amd_win_libm'
$manifest=Get-Content (Join-Path $reference 'SOURCES.json') -Raw | ConvertFrom-Json
foreach($entry in $manifest.files) {
    $source=[IO.File]::ReadAllText((Join-Path $original $entry.file)).Replace("`r`n","`n")
    $sha=[Security.Cryptography.SHA256]::Create()
    try { $actual=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($source))).Replace('-','') }
    finally { $sha.Dispose() }
    if($actual -ne $entry.sha256_lf) { throw "Original AMD source hash mismatch: $($entry.file)" }
}
if(Test-Path -LiteralPath $OutputDirectory) { throw 'Output directory must be new' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
function Hex([string]$value) { return [Convert]::ToUInt64($value,16) }
function Float-Literal([uint64]$value) { return ('uintBitsToFloat(0x{0:x8}u)' -f ($value -band 0xffffffffL)) }
function Pair-Literal([uint64]$value) {
    $d=[BitConverter]::Int64BitsToDouble([long]$value)
    $hi=[single]$d; $lo=[single]($d-[double]$hi)
    return 'vec2('+(Float-Literal ([BitConverter]::ToUInt32([BitConverter]::GetBytes($hi),0)))+','+(Float-Literal ([BitConverter]::ToUInt32([BitConverter]::GetBytes($lo),0)))+')'
}
function Constant([string]$file,[string]$label) {
    $source=[IO.File]::ReadAllText((Join-Path $original $file))
    $m=[regex]::Match($source,'(?m)^\s*'+[regex]::Escape($label)+'\s+D[QD]\s+([0-9a-fA-F]+)h')
    if(!$m.Success) { throw "Missing source constant $label" }
    return Hex $m.Groups[1].Value
}
function Table([string]$file,[string]$label,[int]$count) {
    $source=[IO.File]::ReadAllText((Join-Path $original $file))
    $m=[regex]::Match($source,'(?m)^\s*'+[regex]::Escape($label)+'\s+D[QD]\s+')
    if(!$m.Success) { throw "Missing source table $label" }
    $words=[regex]::Matches($source.Substring($m.Index),'\bD[QD]\s+([0-9a-fA-F]+)h')
    if($words.Count -lt $count) { throw 'Truncated source table' }
    return @(0..($count-1) | ForEach-Object { Hex $words[$_].Groups[1].Value })
}
$prefix="// SPDX-License-Identifier: MIT; AMD source attribution in math_reference/amd_win_libm/LICENSE.txt`n"
foreach($entry in @(@('inverseTable','log_F_inv_dword_table.asm','__log_F_inv_dword'),@('leadTable','log_128_lead_tail_table.asm','__log_128_lead'),@('tailTable','log_128_lead_tail_table.asm','__log_128_tail'))) {
    $values=@(Table $entry[1] $entry[2] 129 | ForEach-Object { Float-Literal $_ })
    $prefix+="const float $($entry[0])[129]=float[129]("+($values -join ',')+");`n"
}
foreach($entry in @(@('nearThreshold','L_real_threshold'),@('nearC0','L_real_ca2'),@('nearC1','L_real_ca1'),@('polyC0','L_real_1_over_3'),@('polyC1','L_real_1_over_2'),@('log2Lead','L_real_log2_lead'),@('log2Tail','L_real_log2_tail'))) {
    $prefix+="const float $($entry[0])="+(Float-Literal (Constant 'logf.asm' $entry[1]))+";`n"
}
$values=@(Table 'two_to_jby64_table.asm' '__two_to_jby64_table' 64 | ForEach-Object { Pair-Literal $_ })
$prefix+="const vec2 expTable[64]=vec2[64]("+($values -join ',')+");`n"
foreach($entry in @(@('expMultiplier','__real_64_by_log2'),@('expStep','__real_log2_by_64'),@('expC0','__real_1_by_6'),@('expC1','__real_1_by_2'))) {
    $prefix+="const vec2 $($entry[0])="+(Pair-Literal (Constant 'expf.asm' $entry[1]))+";`n"
}
$helpers=$prefix+[IO.File]::ReadAllText((Join-Path $reference 'native_math.glsl'))
$utf8=[Text.UTF8Encoding]::new($false)
$test=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'float_math.comp'))
$start=$test.IndexOf('vec2 pairSum'); $end=$test.IndexOf('void main()')
$test=$test.Substring(0,$start)+$helpers+"`n"+$test.Substring($end)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'math.comp'),$test,$utf8)
foreach($kind in @('prepare','finish')) {
    $source=[IO.File]::ReadAllText((Join-Path $root "shaders/hdrvdp3/hdrvdp3_fft_$kind.glsl"))
    $source=$source.Replace('void main() {',$helpers+"`nvoid main() {")
    $source=$source.Replace('log(max(value, 1e-6))','accurateLog(max(value, 1e-6))').Replace('exp(reconstructed)','accurateExp(reconstructed)')
    [IO.File]::WriteAllText((Join-Path $OutputDirectory "$kind.comp"),$source,$utf8)
}
$multiply=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'float_multiply.comp'))
$start=$multiply.IndexOf('uint roundedShift'); $end=$multiply.IndexOf('void main()')
$multiplyHelpers=$multiply.Substring($start,$end-$start)
$source=[IO.File]::ReadAllText((Join-Path $root 'shaders/hdrvdp3/mtf_multiply.comp'))
$source=$source.Replace('void main() {',$multiplyHelpers+"`nvoid main() {")
$source=$source.Replace('complex_val.x * mtf_val','exactProduct(complex_val.x,mtf_val)').Replace('complex_val.y * mtf_val','exactProduct(complex_val.y,mtf_val)')
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'multiply.comp'),$source,$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'origin.txt'),"AMD win-libm commit6121d02aef0c775947d5dc655d7fc4ccfe2144ab; no Windows binary dependency; domain log[1e-6,1e10], exp[-80,80].`n",$utf8)
