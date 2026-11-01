# Native base FFT plus DC-excluded hypot and first column-major maximum.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if (Test-Path $OutputDirectory) { throw 'Fresh output required' }
New-Item -ItemType Directory $OutputDirectory | Out-Null
& "$PSScriptRoot/generate_residual144_shader.ps1" -OutputDirectory "$OutputDirectory/fft"
$s=[IO.File]::ReadAllText("$OutputDirectory/fft/residual144.comp")
$start=$s.IndexOf('void storeForward(')
if ($start -lt 0) { throw 'Missing unused legacy helper' }
$opening=$s.IndexOf('{',$start);$depth=0;$end=-1
for ($i=$opening;$i -lt $s.Length;++$i) {
    if ($s[$i] -eq '{') { ++$depth }
    if ($s[$i] -eq '}') { --$depth; if (!$depth) { $end=$i+1;break } }
}
if ($end -lt 0) { throw 'Unbalanced helper' }
$s=$s.Remove($start,$end-$start)
$s=$s.Replace('writeonly buffer Output { vec2 value[]; }','writeonly buffer Output { float value[]; }')
$assignment='dst.value[batch*144u+i]=entirelyReal?vec2(result[i].x,0.0):result[i];'
if (!$s.Contains($assignment)) { throw 'Unexpected FFT export' }
$s=$s.Replace($assignment,'result[i]=entirelyReal?vec2(result[i].x,0.0):result[i];')
$s=$s.Insert($s.IndexOf('void main(){'),[IO.File]::ReadAllText("$PSScriptRoot/math_reference/hypot_reference.glsl")+"`n")
$export=@'
    for(uint row=0u;row<144u;++row){dst.value[batch*433u+2u*row]=result[row].x;dst.value[batch*433u+2u*row+1u]=result[row].y;}
    float maximum=0.0;uint selected=0u;
    for(uint x=0u;x<w;++x)for(uint y=0u;y<h;++y){
        uint row=y*w+x;vec2 z=result[row];float magnitude=row==0u?0.0:finite_hypot(z.x,z.y);
        dst.value[batch*433u+288u+row]=magnitude;
        if(magnitude>maximum){maximum=magnitude;selected=x*h+y;}
    }
    dst.value[batch*433u+432u]=float(selected);
'@
$s=$s.Insert($s.LastIndexOf('}'),$export+"`n")
[IO.File]::WriteAllText("$OutputDirectory/residual144_extended.comp",$s,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
