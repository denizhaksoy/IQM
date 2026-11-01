# Separate 16x16 base ABI; accepted 10/12 and legacy ABI82 stay unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_base16_fft_shader.ps1" -OutputDirectory "$OutputDirectory/graphs"
$s=[IO.File]::ReadAllText("$OutputDirectory/graphs/base16.comp")
$guard='uint batch=gl_GlobalInvocationID.x;if(batch>=p.batches)return;'
if(!$s.Contains($guard)){throw 'Base16 entrypoint changed'}
$s=$s.Replace($guard,'uint batch=gl_GlobalInvocationID.x;if(batch>=p.n||p.batches!=16u||p.graph!=16u)return;')
$s=$s.Replace('batch*256u+y*16u+x','batch*256u+x*16u+y')
$anchor='    for(uint i=0u;i<256u;++i){dst.value'
$start=$s.IndexOf($anchor);if($start-lt 0){throw 'Base16 output anchor changed'}
$narrow=@'
    bool entirelyReal=true;
    for(uint i=0u;i<256u;++i)if(spectrum[i].y!=0.0)entirelyReal=false;
    if(entirelyReal)for(uint i=0u;i<256u;++i)spectrum[i].y=0.0;
'@
$s=$s.Insert($start,$narrow+"`n")
[IO.File]::WriteAllText("$OutputDirectory/residual256_extended.comp",$s,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
