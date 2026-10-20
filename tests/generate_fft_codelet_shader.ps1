# Generates test-only GLSL translations of FFTW 3.3.10 scalar graph variants.
# Original sources and GPL-2.0-or-later license are preserved in fftw_reference/.
param([string]$OutputDirectory='')
$ErrorActionPreference = 'Stop'
if($OutputDirectory){
    if(Test-Path -LiteralPath $OutputDirectory){throw 'Use a fresh output directory'}
    New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
}
$referenceDir = Join-Path $PSScriptRoot 'fftw_reference'
function Get-BraceBody([string]$source, [int]$opening) {
    $depth=0
    for ($i=$opening; $i -lt $source.Length; ++$i) {
        if ($source[$i] -eq '{') { ++$depth }
        if ($source[$i] -eq '}') { --$depth; if ($depth -eq 0) { return $source.Substring($opening+1,$i-$opening-1) } }
    }
    throw 'Unbalanced source function'
}
$shader = @'
#version 450
// Test-only translation of FFTW 3.3.10 scalar n1_8/n1_16/t2_8 graphs.
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2003, 2007-14 Matteo Frigo
// Copyright (c) 2003, 2007-14 Massachusetts Institute of Technology
// Modified 2026 for IQM numerical testing. See COPYING and original .c files.
layout(local_size_x=32) in;
layout(set=0,binding=0,std430) readonly buffer Input { vec2 value[]; } src;
layout(set=0,binding=1,std430) writeonly buffer Output { vec2 value[]; } dst;
layout(push_constant) uniform Parameters { uint n; uint batches; uint graph; uint fused; } p;
float FMA(float a,float b,float c) { if(p.fused!=0) return fma(a,b,c); precise float product=a*b; return product+c; }
float FNMS(float a,float b,float c) { if(p.fused!=0) return fma(-a,b,c); precise float product=a*b; return c-product; }
float FMS(float a,float b,float c) { if(p.fused!=0) return fma(a,b,-c); precise float product=a*b; return product-c; }
float FNMA(float a,float b,float c) { if(p.fused!=0) return -fma(a,b,c); precise float product=a*b; return -(product+c); }
float sum_products(float a,float b,float c,float d,bool subtract_first) {
    precise float left=a*b; precise float right=c*d;
    if(p.fused==1) return fma(subtract_first?-a:a,b,right);
    if(p.fused==2) return fma(c,d,subtract_first?-left:left);
    return subtract_first?right-left:left+right;
}
'@
foreach ($n in @(8,16)) {
    $source=[IO.File]::ReadAllText((Join-Path $referenceDir "n1_$n.c"))
    $functions=[regex]::Matches($source,"static void n1_$n\(")
    if ($functions.Count -ne 2) { throw 'Expected FMA and non-FMA graph variants' }
    for ($graph=0; $graph -lt 2; ++$graph) {
        $opening=$source.IndexOf('{',$functions[$graph].Index)
        $body=Get-BraceBody $source $opening
        $constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
        $loop=[regex]::Match($body,'for\s*\(')
        if (!$loop.Success) { throw 'Expected vector loop' }
        $loopOpening=$body.IndexOf('{',$loop.Index)
        $operations=Get-BraceBody $body $loopOpening
        $operations=[regex]::Replace($operations,'\bE\b','precise float')
        $operations=[regex]::Replace($operations,'WS\((?:is|os),\s*(\d+)\)','$1')
        if ($graph -eq 1) {
            $operations=[regex]::Replace($operations,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)', {
                param($m)
                $subtract=if($m.Groups[1].Value -eq 'FNMS') {'true'} else {'false'}
                return "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$subtract)"
            })
        }
        if ($operations -match '\b(?:WS|INT|stride|MAKE_VOLATILE_STRIDE)\b') { throw 'Untranslated operation' }
        $shader += "`nvoid graph${n}_${graph}(in float ri[$n],in float ii[$n],out float ro[$n],out float io[$n]) {`n"
        foreach($constant in $constants) { $shader += "const float $($constant.Groups[1].Value) = $($constant.Groups[2].Value);`n" }
        $shader += $operations + "`n}`n"
    }
}
# t2_8 is an in-place twiddle-before-butterfly routine. Its compact table
# stores powers 1,3,7 (not 1,2,4); rs is mapped to contiguous local arrays.
$source=[IO.File]::ReadAllText((Join-Path $referenceDir 't2_8.c'))
$functions=[regex]::Matches($source,'static void t2_8\(')
if($functions.Count -ne 2) { throw 'Expected two t2_8 graphs' }
for($graph=0;$graph -lt 2;++$graph) {
    $body=Get-BraceBody $source ($source.IndexOf('{',$functions[$graph].Index))
    $constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\(')
    $operations=Get-BraceBody $body ($body.IndexOf('{',$loop.Index))
    $operations=[regex]::Replace($operations,'\bE\b','precise float')
    $operations=[regex]::Replace($operations,'WS\(rs,\s*(\d+)\)','$1')
    if($graph -eq 1) {
        $operations=[regex]::Replace($operations,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)', {
            param($m)
            $subtract=if($m.Groups[1].Value -eq 'FNMS') {'true'} else {'false'}
            return "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$subtract)"
        })
    }
    $shader += "`nvoid twiddle8_${graph}(inout float ri[8],inout float ii[8],in float W[6]) {`n"
    foreach($constant in $constants) { $shader += "const float $($constant.Groups[1].Value) = $($constant.Groups[2].Value);`n" }
    $shader += $operations + "`n}`n"
}
# Twiddles are host-generated double trig, then rounded once to float32.
# Their equivalence to FFTW's native table must be established by comparison.
$shader += "`nconst float twiddles[96]=float[96](`n"
$table=@()
for($k=0;$k -lt 16;++$k) {
    foreach($power in @(1,3,7)) {
        $angle=2.0*[Math]::PI*$k*$power/128.0
        $table += ([single][Math]::Cos($angle)).ToString('R',[Globalization.CultureInfo]::InvariantCulture)
        $table += ([single][Math]::Sin($angle)).ToString('R',[Globalization.CultureInfo]::InvariantCulture)
    }
}
$shader += ($table -join ",`n") + "`n);`n"
$shader += @'
vec2 VADD(vec2 a,vec2 b) { return a+b; }
vec2 VSUB(vec2 a,vec2 b) { return a-b; }
vec2 VMUL(vec2 a,vec2 b) { return a*b; }
vec2 VBYI(vec2 a) { return vec2(-a.y,a.x); }
vec2 VFMA(vec2 a,vec2 b,vec2 c) { return fma(a,b,c); }
vec2 VFNMS(vec2 a,vec2 b,vec2 c) { return fma(-a,b,c); }
vec2 VFMAI(vec2 b,vec2 c) { return c+vec2(-b.y,b.x); }
vec2 VFNMSI(vec2 b,vec2 c) { return c-vec2(-b.y,b.x); }
vec2 VZMUL(vec2 a,vec2 b) { precise vec2 t=a.y*b.yx; return vec2(fma(a.x,b.x,-t.x),fma(a.x,b.y,t.y)); }
vec2 VZMULJ(vec2 a,vec2 b) { precise vec2 t=a.y*b.yx; return vec2(fma(a.x,b.x,t.x),fma(a.x,b.y,-t.y)); }
'@
$source=[IO.File]::ReadAllText((Join-Path $referenceDir 't3bv_16.c'))
$functions=[regex]::Matches($source,'static void t3bv_16\(')
$function=$functions[1] # Ordinary generated graph; AVX2 complex products still use FMA.
$body=Get-BraceBody $source ($source.IndexOf('{',$function.Index))
$constants=[regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')
$loop=[regex]::Match($body,'for\s*\(')
$operations=Get-BraceBody $body ($body.IndexOf('{',$loop.Index))
$operations=[regex]::Replace($operations,'\bV\b','precise vec2')
$operations=[regex]::Replace($operations,'LDW\(&\(W\[0\]\)\)','W[0]')
$operations=[regex]::Replace($operations,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{ param($m) 'W['+([int]$m.Groups[1].Value/2)+']' })
$operations=[regex]::Replace($operations,'WS\(rs,\s*(\d+)\)','$1')
$operations=[regex]::Replace($operations,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
$operations=[regex]::Replace($operations,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1] = $2;')
$operations=[regex]::Replace($operations,'LDK\((\w+)\)','vec2($1)')
if($operations -match '\b(?:LD|ST|LDW|WS|TWVL)\b') { throw 'Untranslated SIMD operation' }
$shader += "`nvoid simdTwiddle16(inout vec2 x[16],in vec2 W[4]) {`n"
foreach($constant in $constants) { $shader += "const float $($constant.Groups[1].Value) = $($constant.Groups[2].Value);`n" }
$shader += $operations + "`n}`n"
$table=@()
for($k=0;$k -lt 8;++$k) { foreach($power in @(1,3,9,15)) {
    $angle=2.0*[Math]::PI*$k*$power/128.0
    $a=([single][Math]::Cos($angle)).ToString('R',[Globalization.CultureInfo]::InvariantCulture)
    $b=([single][Math]::Sin($angle)).ToString('R',[Globalization.CultureInfo]::InvariantCulture)
    $table += "vec2($a,$b)"
} }
$shader += "`nconst vec2 simdTwiddles[32]=vec2[32](`n"+($table -join ",`n")+"`n);`n"
foreach($name in @('n2fv_16','t3fv_8','n1fv_128')) {
    $source=[IO.File]::ReadAllText((Join-Path $referenceDir "$name.c"))
    $functions=[regex]::Matches($source,"static void $name\(")
    if($functions.Count -ne 2) { throw 'Expected two SIMD forward graphs' }
    $body=Get-BraceBody $source ($source.IndexOf('{',$functions[1].Index))
    $constants=[regex]::Matches($body,'DVK\((\w+),\s*([^;]+)\);')
    $loop=[regex]::Match($body,'for\s*\(')
    $operations=Get-BraceBody $body ($body.IndexOf('{',$loop.Index))
    $operations=[regex]::Replace($operations,'\bV\b','precise vec2')
    $operations=[regex]::Replace($operations,'LDK\((\w+)\)','vec2($1)')
    $operations=[regex]::Replace($operations,'WS\((?:is|rs|os),\s*(\d+)\)','$1')
    if($name -eq 'n1fv_128') {
        $operations=[regex]::Replace($operations,'LD\(&\(xi\[(\d+)\]\), ivs, &\(xi\[\d+\]\)\)','x[$1]')
        $operations=[regex]::Replace($operations,'ST\(&\(xo\[(\d+)\]\), (.+), ovs, &\(xo\[\d+\]\)\);','y[$1] = $2;')
        $signature='void forwardTail128(in vec2 x[128],out vec2 y[128])'
    } elseif($name -eq 'n2fv_16') {
        $operations=[regex]::Replace($operations,'LD\(&\(xi\[(\d+)\]\), ivs, &\(xi\[\d+\]\)\)','x[$1]')
        $operations=[regex]::Replace($operations,'STM2\(&\(xo\[(\d+)\]\), (\w+), ovs, &\(xo\[\d+\]\)\);',{param($m) 'y['+([int]$m.Groups[1].Value/2)+'] = '+$m.Groups[2].Value+';'})
        $operations=[regex]::Replace($operations,'STN2\([^;]+;','') # AVX2-128 defines this as no-op.
        $signature='void forward16(in vec2 x[16],out vec2 y[16])'
    } else {
        $operations=[regex]::Replace($operations,'LDW\(&\(W\[0\]\)\)','W[0]')
        $operations=[regex]::Replace($operations,'LDW\(&\(W\[TWVL \* (\d+)\]\)\)',{param($m) 'W['+([int]$m.Groups[1].Value/2)+']'})
        $operations=[regex]::Replace($operations,'LD\(&\(x\[(\d+)\]\), ms, &\(x\[\d+\]\)\)','x[$1]')
        $operations=[regex]::Replace($operations,'ST\(&\(x\[(\d+)\]\), (.+), ms, &\(x\[\d+\]\)\);','x[$1] = $2;')
        $signature='void forwardTwiddle8(inout vec2 x[8],in vec2 W[3])'
    }
    if($operations -match '\b(?:LD|ST|STM2|STN2|WS|TWVL|xi|xo)\b') { throw 'Untranslated forward SIMD operation' }
    $shader += "`n$signature {`n"
    foreach($constant in $constants) { $shader += "const float $($constant.Groups[1].Value) = $($constant.Groups[2].Value);`n" }
    $shader += $operations+"`n}`n"
}
$source=[IO.File]::ReadAllText((Join-Path $referenceDir 'r2cf_128.c'))
$functions=[regex]::Matches($source,'static void r2cf_128\(')
if($functions.Count -ne 2) { throw 'Expected two real128 graphs' }
$body=Get-BraceBody $source ($source.IndexOf('{',$functions[1].Index))
$constants=[regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')
$loop=[regex]::Match($body,'for\s*\(')
$operations=Get-BraceBody $body ($body.IndexOf('{',$loop.Index))
$operations=[regex]::Replace($operations,'\bE\b','precise float')
$operations=[regex]::Replace($operations,'WS\((?:rs|csr|csi),\s*(\d+)\)','$1')
$operations=[regex]::Replace($operations,'R([01])\[(\d+)\]',{param($m) 'samples['+(2*[int]$m.Groups[2].Value+[int]$m.Groups[1].Value)+']'})
$operations=[regex]::Replace($operations,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)', {
    param($m)
    $subtract=if($m.Groups[1].Value -eq 'FNMS') {'true'} else {'false'}
    return "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$subtract)"
})
if($operations -match '\b(?:WS|R0|R1|stride)\b') { throw 'Untranslated real FFT operation' }
$shader += "`nvoid real128(in float samples[128],out float Cr[65],out float Ci[65]) {`n"
foreach($constant in $constants) { $shader += "const float $($constant.Groups[1].Value) = $($constant.Groups[2].Value);`n" }
$shader += "Ci[0]=0.0; Ci[64]=0.0;`n"+$operations+"`n}`n"
$shader += @'
vec2 loadForward(uint batch,uint i) {
    uint index=p.graph==9?(batch/65)*16384+(batch%65)*128+i:batch*128+i;
    return src.value[index];
}
void storeForward(uint batch,uint i,vec2 v) {
    if(p.graph!=9) { dst.value[batch*128+i]=v; return; }
    uint matrixBase=(batch/65)*16384,k=batch%65;
    dst.value[matrixBase+i*128+k]=v;
    // Two-dimensional Hermitian symmetry conjugates both frequency axes.
    if(k>0 && k<64) dst.value[matrixBase+((128-i)%128)*128+128-k]=vec2(v.x,-v.y);
}
void main() {
    uint batch=gl_GlobalInvocationID.x; if(batch>=p.batches) return;
    if(p.n==8) {
        float ri[8],ii[8],ro[8],io[8];
        for(uint i=0;i<8;++i) { vec2 v=src.value[batch*8+i]; ri[i]=v.y; ii[i]=v.x; }
        if(p.graph==0) graph8_0(ri,ii,ro,io); else graph8_1(ri,ii,ro,io);
        for(uint i=0;i<8;++i) dst.value[batch*8+i]=vec2(io[i],ro[i]);
    } else if(p.n==16) {
        float ri[16],ii[16],ro[16],io[16];
        for(uint i=0;i<16;++i) { vec2 v=src.value[batch*16+i]; ri[i]=v.y; ii[i]=v.x; }
        if(p.graph==0) graph16_0(ri,ii,ro,io); else graph16_1(ri,ii,ro,io);
        for(uint i=0;i<16;++i) dst.value[batch*16+i]=vec2(io[i],ro[i]);
    } else if(p.n==128) {
        if(p.graph==7 || (p.graph==9 && batch%65>=63)) {
            vec2 x[128],y[128];
            for(uint i=0;i<128;++i) x[i]=loadForward(batch,i);
            forwardTail128(x,y);
            for(uint i=0;i<128;++i) storeForward(batch,i,y[i]);
            return;
        }
        if(p.graph==6 || p.graph==9) {
            vec2 intermediate[128];
            for(uint j=0;j<8;++j) {
                vec2 x[16],y[16];
                for(uint l=0;l<16;++l) x[l]=loadForward(batch,j+8*l);
                forward16(x,y);
                for(uint k=0;k<16;++k) intermediate[j+8*k]=y[k];
            }
            for(uint k=0;k<16;++k) {
                vec2 x[8],W[3];
                for(uint j=0;j<8;++j) x[j]=intermediate[j+8*k];
                for(uint w=0;w<3;++w) W[w]=vec2(twiddles[k*6+2*w],twiddles[k*6+2*w+1]);
                forwardTwiddle8(x,W);
                for(uint j=0;j<8;++j) storeForward(batch,k+16*j,x[j]);
            }
            return;
        }
        if(p.graph==5 || p.graph==8) {
            float samples[128],Cr[65],Ci[65];
            for(uint i=0;i<128;++i) samples[i]=src.value[batch*128+i].x;
            real128(samples,Cr,Ci);
            if(p.graph==8) {
                for(uint k=0;k<65;++k) dst.value[(batch/128)*16384+k*128+batch%128]=vec2(Cr[k],Ci[k]);
                return;
            }
            for(uint i=0;i<128;++i) {
                uint k=i<=64?i:128-i;
                dst.value[batch*128+i]=vec2(Cr[k],i<=64?Ci[k]:-Ci[k]);
            }
            return;
        }
        float realPart[128],imagPart[128];
        if(p.graph==2 || p.graph==4) {
            // Sixteen decimated inverse8 transforms, then eight SIMD inverse16 groups.
            for(uint j=0;j<16;++j) {
                float ri[8],ii[8],ro[8],io[8];
                for(uint l=0;l<8;++l) { vec2 v=src.value[batch*128+j+16*l]; ri[l]=v.y; ii[l]=v.x; }
                graph8_1(ri,ii,ro,io);
                for(uint k=0;k<8;++k) { realPart[j+16*k]=io[k]; imagPart[j+16*k]=ro[k]; }
            }
            for(uint k=0;k<8;++k) {
                vec2 x[16],W[4];
                for(uint j=0;j<16;++j) x[j]=vec2(realPart[j+16*k],imagPart[j+16*k]);
                for(uint w=0;w<4;++w) W[w]=simdTwiddles[k*4+w];
                simdTwiddle16(x,W);
                for(uint j=0;j<16;++j) {
                    uint index=p.graph==4?(batch/128)*16384+(k+8*j)*128+batch%128:batch*128+k+8*j;
                    dst.value[index]=p.graph==4?x[j]/16384.0:x[j];
                }
            }
            return;
        }
        // Eight decimated 16-point transforms, then sixteen twiddle8 groups.
        for(uint j=0;j<8;++j) {
            float ri[16],ii[16],ro[16],io[16];
            for(uint l=0;l<16;++l) { vec2 v=src.value[batch*128+j+8*l]; ri[l]=v.y; ii[l]=v.x; }
            // Keep the already validated leaf path fixed during twiddle tests.
            graph16_1(ri,ii,ro,io);
            for(uint k=0;k<16;++k) { realPart[j+8*k]=ro[k]; imagPart[j+8*k]=io[k]; }
        }
        for(uint k=0;k<16;++k) {
            float ri[8],ii[8],W[6];
            for(uint j=0;j<8;++j) { ri[j]=realPart[j+8*k]; ii[j]=imagPart[j+8*k]; }
            for(uint w=0;w<6;++w) W[w]=twiddles[k*6+w];
            if(p.graph==0) twiddle8_0(ri,ii,W); else twiddle8_1(ri,ii,W);
            for(uint j=0;j<8;++j) {
                uint index=p.graph==3?(batch/128)*16384+(k+16*j)*128+batch%128:batch*128+k+16*j;
                dst.value[index]=vec2(ii[j],ri[j]);
            }
        }
    }
}
'@
$output=if($OutputDirectory){Join-Path $OutputDirectory 'fft128.comp'}else{Join-Path $referenceDir 'fft_codelet_test.comp'}
[IO.File]::WriteAllText($output,$shader,[Text.UTF8Encoding]::new($false))
