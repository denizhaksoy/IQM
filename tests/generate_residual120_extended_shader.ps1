# Diagnostic residual FFT, exact finite norm and first column-major maximum.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_residual120_shader.ps1" -OutputDirectory "$OutputDirectory/fft"
$s=[IO.File]::ReadAllText("$OutputDirectory/fft/residual120.comp")
# Remove an unused legacy export helper whose SSBO element type is vec2.
$start=$s.IndexOf('void storeForward(');if($start -lt 0){throw 'Missing legacy helper'}
$opening=$s.IndexOf('{',$start);$depth=0;$end=-1
for($i=$opening;$i -lt $s.Length;++$i){if($s[$i] -eq '{'){++$depth};if($s[$i] -eq '}'){--$depth;if(!$depth){$end=$i+1;break}}}
if($end -lt 0){throw 'Unbalanced helper'};$s=$s.Remove($start,$end-$start)
if([regex]::Matches($s,'Output\s*\{\s*vec2 value\[\];\s*\}').Count -ne 1){throw 'Unexpected output buffer'}
$s=[regex]::Replace($s,'Output\s*\{\s*vec2 value\[\];\s*\}','Output {float value[];}')
$s=$s.Replace('uint halfH=h/2u+1u;vec2 halfSpectrum[84];','uint halfH=h/2u+1u;vec2 halfSpectrum[84];precise vec2 spectrum[120];')
if([regex]::Matches($s,'dst\.value\[batch\*120u\+').Count -ne 4){throw 'Unexpected FFT exports'}
$s=[regex]::Replace($s,'dst\.value\[batch\*120u\+([^\]]+)\]','spectrum[$1]')
$s=$s.Insert($s.IndexOf('void main(){'),[IO.File]::ReadAllText("$PSScriptRoot/math_reference/hypot_reference.glsl")+"`n")
$export=@'
    for(uint row=0u;row<120u;++row){dst.value[batch*361u+2u*row]=spectrum[row].x;dst.value[batch*361u+2u*row+1u]=spectrum[row].y;}
    float maximum=0;uint selected=0;
    for(uint x=0u;x<w;++x)for(uint y=0u;y<h;++y){
        uint row=y*w+x;vec2 z=spectrum[row];float magnitude=row==0u?0:finite_hypot(z.x,z.y);
        dst.value[batch*361u+240u+row]=magnitude;
        if(magnitude>maximum){maximum=magnitude;selected=x*h+y;}
    }
    dst.value[batch*361u+360u]=float(selected);
'@
$s=$s.Insert($s.LastIndexOf('}'),$export+"`n")
[IO.File]::WriteAllText("$OutputDirectory/residual120_extended.comp",$s,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
