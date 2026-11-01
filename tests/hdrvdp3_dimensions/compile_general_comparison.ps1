param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='',[switch]$RGB)
$ErrorActionPreference='Stop'

if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
if(!$VulkanBin){
    if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK 'Bin'}
    else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}
}
$tests=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
Copy-Item -LiteralPath "$tests/general_comparison.comp" -Destination $OutputDirectory
$output=(Resolve-Path -LiteralPath $OutputDirectory).Path
$reference=Join-Path (Split-Path $tests -Parent) 'shaders/hdrvdp3/hdrvdp3_map_reference.glsl'
$map=[IO.File]::ReadAllText($reference);$begin=$map.IndexOf('float divide_finite(');$end=$map.IndexOf('float exact_sqrt(')
if($begin -lt 0 -or $end -le $begin){throw 'Missing finite divide helper'}
$shader=[IO.File]::ReadAllText("$output/general_comparison.comp")
if($RGB){
 $anchor='uint i=gl_GlobalInvocationID.x;if(i>=p.count)return;'
 if([regex]::Matches($shader,[regex]::Escape($anchor)).Count -ne 1){throw 'Comparison entry changed'}
 $branch=@'
 if(p.mode==3u){
  uint n=p.w*p.h;bool changed=false;
  for(uint c=0u;c<3u;++c){
   float r=uintBitsToFloat(src.words[c*n+i]),t=uintBitsToFloat(src.words[(3u+c)*n+i]);
   precise float d=t-r;
   // Native 0/0 is NaN (false); positive/0 is Inf (true).
   bool channel=r==0.0?t!=0.0:divide_finite(abs(d),r)>0.001;
   changed=changed||channel;
  }
  dst.words[i]=uint(changed);return;
 }
'@
 $shader=$shader.Replace($anchor,$anchor+"`n"+$branch)
}
if([regex]::Matches($shader,'// DIVIDE_HELPER').Count -ne 1){throw 'Missing helper marker'}
[IO.File]::WriteAllText("$output/general_comparison.comp",$shader.Replace('// DIVIDE_HELPER',$map.Substring($begin,$end-$begin)))
Get-FileHash -LiteralPath $reference|ConvertTo-Json|Set-Content "$output/helper_source_hash.json"
& "$VulkanBin/glslc.exe" "$output/general_comparison.comp" -o "$output/default_controls.spv" --target-env=vulkan1.2
if($LASTEXITCODE -ne 0){throw 'GLSL compilation failed'}
& "$VulkanBin/spirv-dis.exe" "$output/default_controls.spv" -o "$output/default_controls.spvasm"
if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
$source=[IO.File]::ReadAllText("$output/default_controls.spvasm")
if($source -notmatch 'OpExecutionMode %main LocalSize 64 1 1'){throw 'Unexpected SPIR-V entrypoint'}
$source=$source.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability DenormPreserve`n               OpCapability RoundingModeRTE")
$source=$source.Replace('OpExecutionMode %main LocalSize 64 1 1',"OpExecutionMode %main LocalSize 64 1 1`n               OpExecutionMode %main DenormPreserve 32`n               OpExecutionMode %main RoundingModeRTE 32")
[IO.File]::WriteAllText("$output/general_comparison.spvasm",$source)
& "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$output/general_comparison.spvasm" -o "$output/general_comparison.spv"
if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
& "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$output/general_comparison.spv"
if($LASTEXITCODE -ne 0){throw 'SPIR-V validation failed'}
Get-FileHash "$output/general_comparison.spv","$output/general_comparison.comp"|ConvertTo-Json|Set-Content "$output/hashes.json"
