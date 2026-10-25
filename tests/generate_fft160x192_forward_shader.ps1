# Diagnostic FFTW3.3.10 translation; GPL-2.0-or-later, fftw_reference/COPYING.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,
      [ValidateSet(0,1)][int]$T1Graph=1,[ValidateSet(0,1)][int]$T3Graph=0)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft160_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/real_axis"
& "$PSScriptRoot/generate_fft_codelet_shader.ps1" -OutputDirectory "$OutputDirectory/helpers"
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
$t1Header=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/mixed192" 't1f.h'
if($t1Header -notmatch '#define BYTWJ BYTWJ1' -or $t1Header -notmatch '#define TWVL TWVL1'){throw 'Unexpected t1 twiddle contract'}
$simdHeader=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/mixed192" 'simd-avx2.h'
if($simdHeader -notmatch '_mm256_fmsubadd_p'){throw 'Unexpected SIMD multiply contract'}
function Body([string]$s,[int]$opening){
    $depth=0;for($i=$opening;$i -lt $s.Length;++$i){
        if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}
    };throw 'Unbalanced codelet'
}
$base=[IO.File]::ReadAllText("$OutputDirectory/helpers/fft128.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()'))
$axis=[IO.File]::ReadAllText("$OutputDirectory/real_axis/forward160_axis.comp")
$begin=$shader.IndexOf('float FMA(');$end=$shader.IndexOf('void graph8_0')
$scalar=$axis.Substring($axis.IndexOf('float FMA('),$axis.IndexOf('void r2cf_8')-$axis.IndexOf('float FMA('))
$scalar+='float FNMA(float a,float b,float c){precise float t=a*b;precise float r=-(t+c);return r;}'
$shader=$shader.Substring(0,$begin)+$scalar+"`n"+$shader.Substring($end)
foreach($name in @('VADD','VSUB','VMUL','VFMAI','VFNMSI')){
    $shader=[regex]::Replace($shader,"(vec2 $name\([^\r\n]+?\)\s*\{)\s*return ([^;]+);\s*\}", '$1 precise vec2 result=$2; return result; }')
}
$shader+=$axis.Substring($axis.IndexOf('void r2cf_8')).Replace('void main()','void firstAxis()')
# t1f.h selects BYTWJ1 (not BYTWJ2): fuse the cosine product into the
# separately rounded sine product, matching simd-avx2.h VZMULJ.
$shader+=@'
vec2 BYTWJ1(vec2 w,vec2 z){return VZMULJ(w,z);}
'@
foreach($name in @('t3fv_8','t1fv_6','n1_4')){
    $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/mixed192" "$name.c"
    $functions=[regex]::Matches($source,"static void $name\(")
    if($functions.Count -ne 2){throw 'Expected two graph variants'}
    # Graph0 is the generated FMA branch, graph1 the ordinary branch.
    # Defaults match the observed native candidate; explicit SIMD macros fuse.
    $graph=if($name -eq 't1fv_6'){$T1Graph}elseif($name -eq 't3fv_8'){$T3Graph}else{1}
    $body=Body $source ($source.IndexOf('{',$functions[$graph].Index))
    $loop=[regex]::Match($body,'for\s*\(')
    $ops=Body $body ($body.IndexOf('{',$loop.Index))
    if($name -eq 'n1_4'){
        $ops=[regex]::Replace($ops,'\bE\b','precise float')
        $ops=[regex]::Replace($ops,'WS\((?:is|os),\s*(\d+)\)','$1')
        $ops=[regex]::Replace($ops,'\b(ri|ii|ro|io)\[([^\]]+)\]',{param($m)
            $buffer=if($m.Groups[1].Value[0] -eq 'r'){'x'}else{'x'}
            $component=if($m.Groups[1].Value[0] -eq 'r'){'x'}else{'y'}
            "$buffer[$($m.Groups[2].Value)].$component"
        })
        $signature='void leaf4(inout vec2 x[4])'
    }else{
        $ops=[regex]::Replace($ops,'\bV\b','precise vec2')
        $ops=[regex]::Replace($ops,'BYTWJ\(&\(W\[TWVL \* (\d+)\]\), (\w+)\)',{param($m)'BYTWJ1(W['+([int]$m.Groups[1].Value/2)+'],'+$m.Groups[2].Value+')'})
        $ops=[regex]::Replace($ops,'BYTWJ\(&\(W\[0\]\), (\w+)\)','BYTWJ1(W[0],$1)')
        $ops=[regex]::Replace($ops,'LDW\(&\(W\[0\]\)\)','W[0]')
        $ops=[regex]::Replace($ops,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m)'W['+([int]$m.Groups[1].Value/2)+']'})
        $ops=[regex]::Replace($ops,'WS\(rs,\s*(\d+)\)','$1')
        $ops=[regex]::Replace($ops,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
        $ops=[regex]::Replace($ops,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1]=$2;')
        $ops=[regex]::Replace($ops,'LDK\((\w+)\)','vec2($1)')
        $signature=if($name -eq 't3fv_8'){'void twiddle8forward(inout vec2 x[8],in vec2 W[3])'}else{'void twiddle6forward(inout vec2 x[6],in vec2 W[5])'}
    }
    if($ops -match '\b(?:LD|ST|LDW|WS|TWVL|BYTWJ|INT)\b'){throw 'Untranslated graph'}
    $shader+="`n$signature{`n"
    foreach($c in [regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
    $shader+=$ops+"`n}`n"
}
function Literal([single]$v){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($v),0)}
foreach($size in @(24,192)){
    $powers=if($size -eq 24){@(1,2,3,4,5)}else{@(1,3,7)}
    $count=if($size -eq 24){4}else{24};$table=@()
    for($k=0;$k -lt $count;++$k){foreach($power in $powers){
        $phase=($k*$power)%$size;$angle=2*[Math]::PI*$phase/$size
        $re=[single][Math]::Cos($angle);$im=[single][Math]::Sin($angle)
        if($phase -eq 0){$re=1;$im=0};if($phase -eq $size/4){$re=0;$im=1}
        if($phase -eq $size/2){$re=-1;$im=0};if($phase -eq 3*$size/4){$re=0;$im=-1}
        $table+='vec2('+(Literal $re)+','+(Literal $im)+')'
    }}
    $shader+="const vec2 twiddle$size[$($table.Count)]=vec2[$($table.Count)]("+($table -join ',')+");`n"
}
$shader+=@'
void main(){
    if(p.graph==3u){firstAxis();return;}
    uint batch=gl_GlobalInvocationID.x;if(batch>=81u)return;
    vec2 intermediate[192];
    for(int j=0;j<8;++j){
        vec2 inner[24];
        for(int a=0;a<6;++a){
            vec2 x[4];for(int b=0;b<4;++b)x[b]=src.value[uint(j+8*(a+6*b))*81u+batch];
            leaf4(x);for(int b=0;b<4;++b)inner[a+6*b]=x[b];
        }
        for(int b=0;b<4;++b){
            vec2 x[6],W[5];for(int a=0;a<6;++a)x[a]=inner[a+6*b];
            for(int w=0;w<5;++w)W[w]=twiddle24[b*5+w];
            twiddle6forward(x,W);
            for(int a=0;a<6;++a)intermediate[j+8*(b+4*a)]=x[a];
        }
    }
    for(int k=0;k<24;++k){
        vec2 x[8],W[3];for(int j=0;j<8;++j)x[j]=intermediate[j+8*k];
        for(int w=0;w<3;++w)W[w]=twiddle192[k*3+w];
        twiddle8forward(x,W);
        for(int j=0;j<8;++j){
            uint frequency=uint(k+24*j);dst.value[frequency*160u+batch]=x[j];
            if(batch>0u&&batch<80u)dst.value[((192u-frequency)%192u)*160u+160u-batch]=vec2(x[j].x,-x[j].y);
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/forward160x192.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
