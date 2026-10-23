# Isolated batched DC-output -> FFT/magnitude/first-max contract,ABI84.
# MapABI emits ABI85 with spectrum/padding output for the MapGPU interface.
# FFTW-derived codelets retain GPL-2.0-or-later notices and COPYING.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$MapABI)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_rectangular_base_shader.ps1" -OutputDirectory "$OutputDirectory/components"
$source=[IO.File]::ReadAllText("$OutputDirectory/components/rectangular_base.comp")
$source=$source.Replace('Output {vec2 value[];}','Output {float value[];}')
$source=$source.Replace('rectangular_base_abi=83u','rectangular_base_abi=84u')
$source=$source.Replace('if(rectangular_base_abi!=83u||gl_GlobalInvocationID.x!=0u)return;',
 'uint batch=gl_GlobalInvocationID.x;if(rectangular_base_abi!=84u||batch>=p.batches)return;')
$source=$source.Replace('uint h=p.n,w=p.batches;','uint h=p.n,w=p.graph;')
$source=$source.Replace('src.value[x*h+y].x','src.value[batch*128u+y*w+x].x')
$last='    for(uint y=0u;y<h;++y)for(uint x=0u;x<w;++x)dst.value[x*h+y]=spectrum[y*w+x];'
if(!$source.Contains($last)){throw 'Missing rectangular spectrum export'}
$export=@'
    for(uint row=0u;row<128u;++row){
        dst.value[batch*385u+2u*row]=spectrum[row].x;
        dst.value[batch*385u+2u*row+1u]=spectrum[row].y;
    }
    float maximum=0;uint selected=0;
    for(uint x=0u;x<w;++x)for(uint y=0u;y<h;++y){
        uint row=y*w+x;vec2 z=spectrum[row];float magnitude=row==0u?0:finite_hypot(z.x,z.y);
        dst.value[batch*385u+256u+row]=magnitude;
        if(magnitude>maximum){maximum=magnitude;selected=x*h+y;}
    }
    dst.value[batch*385u+384u]=float(selected);
'@
$source=$source.Replace($last,$export)
if($MapABI){
    $source=$source.Replace('rectangular_base_abi=84u','rectangular_base_abi=85u').Replace('rectangular_base_abi!=84u','rectangular_base_abi!=85u')
    $source=$source.Replace('uint h=p.n,w=p.graph;','uint h=p.fused,w=p.graph;')
    $mapExport=@'
    for(uint row=0u;row<128u;++row){
        dst.value[batch*512u+2u*row]=spectrum[row].x;
        dst.value[batch*512u+2u*row+1u]=spectrum[row].y;
    }
    for(uint k=256u;k<512u;++k)dst.value[batch*512u+k]=0;
'@
    $source=$source.Replace($export,$mapExport)
}
[IO.File]::WriteAllText("$OutputDirectory/rectangular_residual.comp",$source,[Text.UTF8Encoding]::new($false))
Copy-Item "$PSScriptRoot/fftw_reference/COPYING" "$OutputDirectory/COPYING"
