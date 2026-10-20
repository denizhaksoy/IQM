# Finite positive power experiment. Tables come from mathematical definitions;
# this generator does not load DLLs or any reference input/output samples.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory) { throw 'Output directory must be new' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
function Pair([double]$v) {
    $hi=[single]$v; $lo=[single]($v-[double]$hi)
    $a=[BitConverter]::ToUInt32([BitConverter]::GetBytes($hi),0)
    $b=[BitConverter]::ToUInt32([BitConverter]::GetBytes($lo),0)
    return ('vec2(uintBitsToFloat(0x{0:x8}u),uintBitsToFloat(0x{1:x8}u))' -f $a,$b)
}
function Triple([double]$v) {
    $hi=[single]$v;$mid=[single]($v-[double]$hi);$lo=[single](($v-[double]$hi)-[double]$mid)
    $parts=foreach($part in @($hi,$mid,$lo)){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($part),0)}
    return 'vec3('+($parts -join ',')+')'
}
$helpers=Get-Content (Join-Path $PSScriptRoot 'float_math.comp') -Raw
$helpers=$helpers.Substring($helpers.IndexOf('vec2 pairSum'),$helpers.IndexOf('const vec2 LN2')-$helpers.IndexOf('vec2 pairSum'))
foreach($kind in @('powerInverse','powerLog','powerExp')) {
    $count=257; if($kind -eq 'powerExp') { $count=64 }
    $values=for($i=0;$i -lt $count;$i++) {
        $v=switch($kind) {
            'powerInverse' { 2.0/(1.0+$i/256.0) }
            'powerLog' { [Math]::Log(1.0+$i/256.0) }
            'powerExp' { [Math]::Pow(2.0,$i/64.0) }
        }
        Pair $v
    }
    $helpers+="const vec2 ${kind}[$count]=vec2[$count]("+($values -join ',')+");`n"
    $triples=for($i=0;$i -lt $count;$i++) {
        $v=switch($kind){'powerInverse'{2.0/(1.0+$i/256.0)} 'powerLog'{[Math]::Log(1.0+$i/256.0)} 'powerExp'{[Math]::Pow(2.0,$i/64.0)}}
        Triple $v
    }
    $helpers+="const vec3 ${kind}3[$count]=vec3[$count]("+($triples -join ',')+");`n"
}
$constants=@{oneThird=1.0/3; powerLn2=[Math]::Log(2); powerStep=[Math]::Log(2)/64;
    oneSixth=1.0/6; powerMultiplier=64/[Math]::Log(2);
    nearA=1.0/80; nearB=1.0/2304; nearC=1.0/12; nearD=1.0/448}
foreach($name in ($constants.Keys | Sort-Object)) {
    $helpers+="const vec2 $name="+(Pair $constants[$name])+";`n"
    $helpers+="const vec3 ${name}3="+(Triple $constants[$name])+";`n"
}
$helpers+=Get-Content (Join-Path $PSScriptRoot 'math_reference/power_triple_reference.glsl') -Raw
$helpers+=Get-Content (Join-Path $PSScriptRoot 'math_reference/power_reference.glsl') -Raw
$helpers+=Get-Content (Join-Path $PSScriptRoot 'math_reference/hypot_reference.glsl') -Raw
# Reuse the already source-backed native single log for final Q. Keep the
# compensated helpers only once when composing the two math implementations.
& (Join-Path $PSScriptRoot 'generate_reference_math_shaders.ps1') -OutputDirectory (Join-Path $OutputDirectory 'source_math')
$logSource=Get-Content (Join-Path $OutputDirectory 'source_math/math.comp') -Raw
$begin=$logSource.IndexOf('// SPDX-License-Identifier: MIT; AMD source attribution')
$end=$logSource.IndexOf('vec2 pairSum')
$helpers+=$logSource.Substring($begin,$end-$begin)
$log10Path=Join-Path $PSScriptRoot 'math_reference/amd_win_libm/log10_128_lead_tail_table.asm'
$log10Source=Get-Content -LiteralPath $log10Path -Raw
foreach($entry in @(@('log10Lead','__log10_128_lead'),@('log10Tail','__log10_128_tail'))) {
    $match=[regex]::Match($log10Source,'(?m)^'+$entry[1]+':')
    if(!$match.Success) { throw 'Missing log10 source table' }
    $words=[regex]::Matches($log10Source.Substring($match.Index),'\bDD\s+([0-9a-fA-F]+)h')
    if($words.Count -lt 129) { throw 'Truncated log10 source table' }
    $values=for($i=0;$i -lt 129;++$i) { 'uintBitsToFloat(0x'+$words[$i].Groups[1].Value+'u)' }
    $helpers+="const float $($entry[0])[129]=float[129]("+($values -join ',')+");`n"
}
function SingleLiteral([single]$v) { return ('uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($v),0)) }
foreach($entry in @(@('log10E',[Math]::Log10([Math]::E)),@('log10Two',[Math]::Log10(2)))) {
    if($entry[0] -eq 'log10Two') {
        $helpers+="const float log10TwoLead=log10Lead[128];`nconst float log10TwoTail=log10Tail[128];`n"
        continue
    }
    $value=[double]$entry[1]; $rounded=[single]$value
    $bits=[BitConverter]::ToUInt32([BitConverter]::GetBytes($rounded),0) -band 0xffff0000L
    $lead=[BitConverter]::ToSingle([BitConverter]::GetBytes([uint32]$bits),0)
    $helpers+="const float $($entry[0])Lead="+(SingleLiteral $lead)+";`n"
    $helpers+="const float $($entry[0])Tail="+(SingleLiteral ([single]($value-$lead)))+";`n"
    if($entry[0] -eq 'log10E') { $helpers+="const float log10E="+(SingleLiteral $rounded)+";`n" }
}
$begin=$logSource.IndexOf('float nearOneRatio')
$end=$logSource.IndexOf('void main()')
$helpers+=$logSource.Substring($begin,$end-$begin)
$helpers+=Get-Content (Join-Path $PSScriptRoot 'math_reference/log10_reference.glsl') -Raw
$header=Get-Content (Join-Path $PSScriptRoot 'float_power_probe.comp') -Raw
$header=$header.Substring(0,$header.IndexOf('void main()'))
$main=@'
void main() {
    uint batch=gl_GlobalInvocationID.x; if(batch>=p.batches) return;
    for(uint i=0;i<p.n;++i) {
        uint index=batch*p.n+i; vec2 v=src.value[index];
        float result=nativePower(v.x,v.y); dst.value[index]=vec2(result,result);
    }
}
'@
$utf8=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'power.comp'),$header+$helpers+$main,$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'power_helpers.glsl'),$helpers,$utf8)
$log10Main=$main.Replace('nativePower(v.x,v.y)','accurateLog10(v.x)')
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'log10.comp'),$header+$helpers+$log10Main,$utf8)
$product=Get-Content (Join-Path $PSScriptRoot 'float_multiply.comp') -Raw
$product=$product.Substring($product.IndexOf('uint roundedShift'),$product.IndexOf('void main()')-$product.IndexOf('uint roundedShift'))
$map=Get-Content (Join-Path $PSScriptRoot '../shaders/hdrvdp3/hdrvdp3_map_reference.glsl') -Raw
$map=$map.Replace('// Generator injects the positive power and exact product helpers here.',$helpers+$product)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'map.comp'),$map,$utf8)
