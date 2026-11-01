# Diagnostic square profile only. Accepted mixed IO generator stays unchanged.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_mixed_filter_io_shader.ps1" -OutputDirectory "$OutputDirectory/base"
$source=[IO.File]::ReadAllText("$OutputDirectory/base/mixed_filter_io.comp")
$anchor='uint i=gl_GlobalInvocationID.x;if(i>=p.count)return;'
if(!$source.Contains($anchor)){throw 'IO entry anchor changed'}
# Octave converts an entirely real complex result to a real matrix after
# FFT/product/inverse. The aliased in-place call uses only invocation zero.
$narrow=@'
 if(p.mode==5u){
  if(i!=0u)return;
  bool entirelyReal=true;
  for(uint k=0u;k<p.count;++k)if(src.value[2u*k+1u]!=0.0)entirelyReal=false;
  if(entirelyReal)for(uint k=0u;k<p.count;++k)dst.value[2u*k+1u]=0.0;
  return;
 }
'@
$source=$source.Replace($anchor,$anchor+"`n"+$narrow)
[IO.File]::WriteAllText("$OutputDirectory/square_filter_io.comp",$source,[Text.UTF8Encoding]::new($false))
