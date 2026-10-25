# Diagnostic FFTW3.3.10 graphs, GPL-2.0-or-later; fftw_reference/COPYING.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet(0,1)][int]$LeafGraph=0,[ValidateSet(0,1)][int]$TwiddleGraph=1,[ValidateSet(0,1)][int]$ProductsOrder=0,[switch]$PreciseVectorHelpers)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft192_inverse_axis_shader.ps1" -OutputDirectory "$OutputDirectory/first_axis"
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$s,[int]$opening){$depth=0;for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}};throw 'Unbalanced graph'}
$base=[IO.File]::ReadAllText("$OutputDirectory/first_axis/inverse192_axis.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()'))
if(!$PreciseVectorHelpers){
    # Match native compiler contraction of this complex160 graph. Keep scalar
    # helpers precise: the independent real192 axis requires those boundaries.
    foreach($name in @('VADD','VSUB','VMUL','VFMAI','VFNMSI')){
        $shader=[regex]::Replace($shader,"(vec2 $name\([^\r\n]+?\)\s*\{)\s*precise vec2 result=([^;]+);\s*return result;\s*\}",'$1 return $2; }')
    }
}
$shader+="`nvec2 VFMS(vec2 a,vec2 b,vec2 c){return fma(a,b,-c);}`n"
$shader+="vec2 sumVecProducts(vec2 a,vec2 b,vec2 c,vec2 d,bool minusFirst){precise vec2 t=a*b;return fma(c,d,minusFirst?-t:t);}`n"
$shader+=$base.Substring($base.IndexOf('void main()')).Replace('void main()','void firstAxis()')
$shader+='void leaf8(in vec2 x[8],out vec2 y[8]){float ri[8],ii[8],ro[8],io[8];for(int i=0;i<8;++i){ri[i]=x[i].y;ii[i]=x[i].x;}graph8_1(ri,ii,ro,io);for(int i=0;i<8;++i)y[i]=vec2(io[i],ro[i]);}'
foreach($name in @('t3bv_20')){
    $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/complex160" "$name.c"
    $f=[regex]::Matches($source,"static void $name\(");if($f.Count -ne 2){throw 'Expected two graphs'}
    $g=if($name -eq 'n2fv_8'){$LeafGraph}else{$TwiddleGraph}
    $body=Body $source ($source.IndexOf('{',$f[$g].Index));$loop=[regex]::Match($body,'for\s*\(')
    $ops=Body $body ($body.IndexOf('{',$loop.Index))
    if($ProductsOrder -eq 1){
        $ops=[regex]::Replace($ops,'\b(VFMA|VFNMS)\(LDK\((\w+)\),\s*(\w+),\s*VMUL\(LDK\((\w+)\),\s*(\w+)\)\)',{
            param($m);$minus=if($m.Groups[1].Value -eq 'VFNMS'){'true'}else{'false'}
            "sumVecProducts(vec2($($m.Groups[2].Value)),$($m.Groups[3].Value),vec2($($m.Groups[4].Value)),$($m.Groups[5].Value),$minus)"
        })
    }
    $ops=[regex]::Replace($ops,'\bV\b','precise vec2')
    $ops=[regex]::Replace($ops,'WS\((?:is|os|rs),\s*(\d+)\)','$1')
    $ops=[regex]::Replace($ops,'LDK\((\w+)\)','vec2($1)')
    if($name -eq 'n2fv_8'){
        $ops=[regex]::Replace($ops,'LD\(&\(xi\[(\d+)\]\), ivs, &\(xi\[\d+\]\)\)','x[$1]')
        $ops=[regex]::Replace($ops,'STM2\(&\(xo\[(\d+)\]\), (\w+), ovs, &\(xo\[\d+\]\)\);',{param($m)'y['+([int]$m.Groups[1].Value/2)+']='+$m.Groups[2].Value+';'})
        $ops=[regex]::Replace($ops,'STN2\([^;]+;','')
        $signature='void leaf8(in vec2 x[8],out vec2 y[8])'
    }else{
        $powers=@([regex]::Matches($source.Substring($f[1].Index),'VTW\(0, (\d+)\)')|ForEach-Object {$_.Groups[1].Value})
        if(($powers -join ',') -ne '1,3,9,19'){throw 'Unexpected descriptor'}
        $ops=[regex]::Replace($ops,'LDW\(&\(W\[0\]\)\)','W[0]')
        $ops=[regex]::Replace($ops,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m)'W['+([int]$m.Groups[1].Value/2)+']'})
        $ops=[regex]::Replace($ops,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
        $ops=[regex]::Replace($ops,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1]=$2;')
        $signature='void twiddle20backward(inout vec2 x[20],in vec2 W[4])'
    }
    if($ops -match '\b(?:LD|ST|STM2|STN2|WS|TWVL|xi|xo)\b'){throw 'Untranslated SIMD operation'}
    $shader+="`n$signature{`n"
    foreach($c in [regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
    $shader+=$ops+"`n}`n"
}
function Literal([single]$v){'uintBitsToFloat(0x{0:x8}u)' -f [BitConverter]::ToUInt32([BitConverter]::GetBytes($v),0)}
$table=@();for($k=0;$k -lt 8;++$k){foreach($power in @(1,3,9,19)){
    $phase=($k*$power)%160;$angle=2*[Math]::PI*$phase/160
    $re=[single][Math]::Cos($angle);$im=[single][Math]::Sin($angle)
    if($phase -eq 0){$re=1;$im=0};if($phase -eq 40){$re=0;$im=1};if($phase -eq 80){$re=-1;$im=0};if($phase -eq 120){$re=0;$im=-1}
    $table+='vec2('+(Literal $re)+','+(Literal $im)+')'
}}
$shader+="const vec2 complexTwiddle160[32]=vec2[32]("+($table -join ',')+");`n"
$normalization=[IO.File]::ReadAllText("$PSScriptRoot/mixed_normalization.comp")
$shader+=$normalization.Substring($normalization.IndexOf('uint divideRounded'),$normalization.IndexOf('void main()')-$normalization.IndexOf('uint divideRounded'))
$shader+=@'
void main(){
    if(p.graph==3u){firstAxis();return;}
    uint batch=gl_GlobalInvocationID.x;if(batch>=192u)return;
    vec2 intermediate[160];
    for(int j=0;j<20;++j){
        vec2 x[8],y[8];for(int l=0;l<8;++l)x[l]=src.value[batch*160u+uint(j+20*l)];
        leaf8(x,y);for(int k=0;k<8;++k)intermediate[j+20*k]=y[k];
    }
    for(int k=0;k<8;++k){
        vec2 x[20],W[4];for(int j=0;j<20;++j)x[j]=intermediate[j+20*k];
        for(int w=0;w<4;++w)W[w]=complexTwiddle160[k*4+w];
        twiddle20backward(x,W);
        for(int j=0;j<20;++j){uint f=uint(k+8*j);vec2 z=x[j];
            if(p.fused!=0u)z=vec2(uintBitsToFloat(normalizeWord(floatBitsToUint(z.x))),uintBitsToFloat(normalizeWord(floatBitsToUint(z.y))));
            dst.value[f*192u+batch]=z;
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/inverse192x160.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
