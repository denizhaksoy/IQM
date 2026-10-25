# Scalar small residual FFT graphs, FFTW3.3.10 GPL-2.0-or-later.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_fft192_inverse_axis_shader.ps1" -OutputDirectory "$OutputDirectory/scalar"
& "$PSScriptRoot/generate_fft192_forward_axis_shader.ps1" -OutputDirectory "$OutputDirectory/real"
. "$PSScriptRoot/fftw_reference/read_runtime_source.ps1"
function Body([string]$s,[int]$opening){$depth=0;for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){return $s.Substring($opening+1,$i-$opening-1)}}};throw 'Unbalanced graph'}
$base=[IO.File]::ReadAllText("$OutputDirectory/scalar/inverse192_axis.comp")
$shader=$base.Substring(0,$base.IndexOf('void main()')).Replace('local_size_x=32','local_size_x=64')
$real=[IO.File]::ReadAllText("$OutputDirectory/real/forward192_axis.comp")
$shader+=$real.Substring($real.IndexOf('void r2cf_8'),$real.IndexOf('void main()')-$real.IndexOf('void r2cf_8'))
foreach($name in @('r2cf_10','n1_10')){
    $source=Read-IQMFFTWSource "$PSScriptRoot/fftw_reference/residual120" "$name.c"
    $f=[regex]::Matches($source,"static void $name\(");if($f.Count -ne 2){throw 'Expected two graphs'}
    $body=Body $source ($source.IndexOf('{',$f[1].Index));$loop=[regex]::Match($body,'for\s*\(')
    $ops=Body $body ($body.IndexOf('{',$loop.Index));$ops=[regex]::Replace($ops,'\bE\b','precise float')
    $ops=[regex]::Replace($ops,'WS\((?:rs|csr|csi|is|os),\s*(\d+)\)','$1')
    $ops=[regex]::Replace($ops,'R0\[(\d+)\]','samples[2*$1]');$ops=[regex]::Replace($ops,'R1\[(\d+)\]','samples[2*$1+1]')
    $ops=[regex]::Replace($ops,'\b(FMA|FNMS)\((\w+),\s*(\w+),\s*(\w+)\s*\*\s*(\w+)\)',{
        param($m);$minus=if($m.Groups[1].Value -eq 'FNMS'){'true'}else{'false'}
        "sum_products($($m.Groups[2].Value),$($m.Groups[3].Value),$($m.Groups[4].Value),$($m.Groups[5].Value),$minus)"
    })
    if($ops -match '\b(?:WS|stride|INT)\b'){throw 'Untranslated scalar graph'}
    $signature=if($name -eq 'r2cf_10'){'void real10(in float samples[10],out float Cr[6],out float Ci[6])'}else{'void leaf10(in float ri[10],in float ii[10],out float ro[10],out float io[10])'}
    $shader+="`n$signature{`n"
    foreach($c in [regex]::Matches($body,'DK\((\w+),\s*([^;]+)\);')){$shader+="const float $($c.Groups[1].Value)=$($c.Groups[2].Value);`n"}
    $shader+=$ops+"`n}`n"
}
$shader+=@'
void main(){
    uint batch=gl_GlobalInvocationID.x;if(batch>=p.n)return;
    uint h=p.batches,w=p.graph;if(!((h==10u&&w==12u)||(h==12u&&w==10u)))return;
    uint halfH=h/2u+1u;vec2 halfSpectrum[84];
    for(uint x=0u;x<w;++x){
        if(h==10u){float samples[10],r[6],im[6];for(uint y=0u;y<h;++y)samples[y]=src.value[batch*120u+x*h+y].x;
            real10(samples,r,im);for(uint y=0u;y<halfH;++y)halfSpectrum[x*halfH+y]=vec2(r[y],y==0u||y==5u?0:im[y]);
        }else{float samples[12],r[7],im[7];for(uint y=0u;y<h;++y)samples[y]=src.value[batch*120u+x*h+y].x;
            r2cf_12(samples,r,im);for(uint y=0u;y<halfH;++y)halfSpectrum[x*halfH+y]=vec2(r[y],y==0u||y==6u?0:im[y]);
        }
    }
    for(uint y=0u;y<halfH;++y){
        if(w==12u){float ri[12],ii[12],ro[12],io[12];for(uint x=0u;x<w;++x){ri[x]=halfSpectrum[x*halfH+y].x;ii[x]=halfSpectrum[x*halfH+y].y;}
            leaf12(ri,ii,ro,io);for(uint x=0u;x<w;++x){dst.value[batch*120u+y*w+x]=vec2(ro[x],io[x]);
                if(y>0u&&y<h/2u)dst.value[batch*120u+(h-y)*w+(w-x)%w]=vec2(ro[x],-io[x]);}
        }else{float ri[10],ii[10],ro[10],io[10];for(uint x=0u;x<w;++x){ri[x]=halfSpectrum[x*halfH+y].x;ii[x]=halfSpectrum[x*halfH+y].y;}
            leaf10(ri,ii,ro,io);for(uint x=0u;x<w;++x){dst.value[batch*120u+y*w+x]=vec2(ro[x],io[x]);
                if(y>0u&&y<h/2u)dst.value[batch*120u+(h-y)*w+(w-x)%w]=vec2(ro[x],-io[x]);}
        }
    }
}
'@
[IO.File]::WriteAllText("$OutputDirectory/residual120.comp",$shader,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
