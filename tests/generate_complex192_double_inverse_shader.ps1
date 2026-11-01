# FFTW3.3.10 diagnostic SIMD graph translation; GPL-2.0-or-later/COPYING.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,
 [Parameter(Mandatory=$true)][string]$TwiddleFile,
 [ValidateSet(0,1)][int]$T1Graph=0,[ValidateSet(0,1)][int]$T3Graph=0)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$s,[int]$opening){$depth=0;for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}};throw 'Unbalanced codelet'}
$shader=@'
#version 450
#extension GL_ARB_gpu_shader_fp64 : require
layout(local_size_x=16) in;
layout(set=0,binding=0,std430) readonly buffer Input {dvec2 value[];} src;
layout(set=0,binding=1,std430) writeonly buffer Output {dvec2 value[];} dst;
layout(push_constant) uniform Push {uint n,batches,graph,reserved;} p;
dvec2 VADD(dvec2 a,dvec2 b){precise dvec2 r=a+b;return r;}
dvec2 VSUB(dvec2 a,dvec2 b){precise dvec2 r=a-b;return r;}
dvec2 VMUL(dvec2 a,dvec2 b){precise dvec2 r=a*b;return r;}
dvec2 VBYI(dvec2 a){return dvec2(-a.y,a.x);}
dvec2 VFMA(dvec2 a,dvec2 b,dvec2 c){return fma(a,b,c);}
dvec2 VFNMS(dvec2 a,dvec2 b,dvec2 c){return fma(-a,b,c);}
dvec2 VFMAI(dvec2 b,dvec2 c){precise dvec2 r=c+dvec2(-b.y,b.x);return r;}
dvec2 VFNMSI(dvec2 b,dvec2 c){precise dvec2 r=c-dvec2(-b.y,b.x);return r;}
dvec2 VZMUL(dvec2 a,dvec2 b){precise dvec2 t=a.y*b.yx;return dvec2(fma(a.x,b.x,-t.x),fma(a.x,b.y,t.y));}
dvec2 VZMULJ(dvec2 a,dvec2 b){precise dvec2 t=a.y*b.yx;return dvec2(fma(a.x,b.x,t.x),fma(a.x,b.y,-t.y));}
dvec2 BYTW1(dvec2 w,dvec2 z){return VZMUL(w,z);}
'@
foreach($name in 't3bv_8','t1bv_12'){
 $dir='double192_inverse'
 $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/$dir" "$name.c"
 $functions=[regex]::Matches($source,"static void $name\(");if($functions.Count -ne 2){throw 'Expected two source variants'}
 $variant=if($name -eq 't3bv_8'){$T3Graph}else{$T1Graph}
 $body=Body $source ($source.IndexOf('{',$functions[$variant].Index));$loop=[regex]::Match($body,'for\s*\(');$ops=Body $body ($body.IndexOf('{',$loop.Index))
 $ops=[regex]::Replace($ops,'\bV\b','precise dvec2')
 $ops=[regex]::Replace($ops,'BYTW\(&\(W\[TWVL \* (\d+)\]\), (\w+)\)',{param($m)'BYTW1(W['+([int]$m.Groups[1].Value/2)+'],'+$m.Groups[2].Value+')'})
 $ops=[regex]::Replace($ops,'BYTW\(&\(W\[0\]\), (\w+)\)','BYTW1(W[0],$1)')
 $ops=[regex]::Replace($ops,'LDW\(&\(W\[0\]\)\)','W[0]')
 $ops=[regex]::Replace($ops,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m)'W['+([int]$m.Groups[1].Value/2)+']'})
 $ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','$1')
 $ops=[regex]::Replace($ops,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
 $ops=[regex]::Replace($ops,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1]=$2;')
 $ops=[regex]::Replace($ops,'LDK\((\w+)\)','dvec2($1)')
 if($ops -match '\b(?:LD|ST|LDW|WS|TWVL|BYTW|INT)\b'){throw 'Untranslated SIMD operation'}
 $size=if($name -eq 't3bv_8'){8}else{12};$tw=if($size -eq 8){3}else{11}
 $shader+="`nvoid graph$size(inout dvec2 x[$size],in dvec2 W[$tw]){`n"
 foreach($c in [regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')){$shader+="const double $($c.Groups[1].Value)=$($c.Groups[2].Value)lf;`n"}
 $shader+=$ops+"`n}`n"
}
function Literal([double]$v){$s=$v.ToString('R',[Globalization.CultureInfo]::InvariantCulture);if($s -notmatch '[.eE]'){$s+='.0'};return $s+'lf'}
$bytes=[IO.File]::ReadAllBytes((Resolve-Path $TwiddleFile));if($bytes.Length -ne 188*8){throw 'Invalid complex192 twiddles'};$offset=0
foreach($entry in @(@(24,22),@(192,72))){$size=$entry[0];$count=$entry[1];$values=@();for($i=0;$i -lt $count;++$i){$a=Literal ([BitConverter]::ToDouble($bytes,$offset));$b=Literal ([BitConverter]::ToDouble($bytes,$offset+8));$offset+=16;$values+="dvec2($a,$b)"};$shader+="const dvec2 twiddle$size[$count]=dvec2[$count]("+($values -join ',')+");`n"}
$shader+=[IO.File]::ReadAllText("$PSScriptRoot/complex192_double_inverse_main.glsl")
[IO.File]::WriteAllText("$OutputDirectory/complex192_double_inverse.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
