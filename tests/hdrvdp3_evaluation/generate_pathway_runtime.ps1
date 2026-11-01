# Source-only mixed-pathway package; no validation exports/native installation.
param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin='',[string]$CMakeGenerator='Visual Studio 18 2026')
$ErrorActionPreference='Stop';$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh source-build directory required'}
if(!$VulkanBin){if($env:VULKAN_SDK){$VulkanBin=Join-Path $env:VULKAN_SDK Bin}else{$VulkanBin=Split-Path (Get-Command glslc.exe -ErrorAction Stop).Source -Parent}}
foreach($tool in @('glslc.exe','glslangValidator.exe','spirv-as.exe','spirv-dis.exe','spirv-val.exe')){if(!(Test-Path "$VulkanBin/$tool")){throw "Missing Vulkan tool:$tool"}}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName;$tests=Join-Path $repo tests
function Generate([string]$Name,[string]$Script,[string[]]$Extra=@()){
 $old=$ErrorActionPreference;$ErrorActionPreference='Continue';& powershell -NoProfile -ExecutionPolicy Bypass -File $Script -OutputDirectory "$out/$Name" @Extra *> "$out/$Name.log";$code=$LASTEXITCODE;$ErrorActionPreference=$old
 if($code -ne 0){throw "Source generation failed:$Name;see $out/$Name.log"};Write-Output "$Name generated"
}
function Native([string]$Name,[string]$Exe,[string[]]$Arguments){$old=$ErrorActionPreference;$ErrorActionPreference='Continue';& $Exe @Arguments *> "$out/$Name.log";$code=$LASTEXITCODE;$ErrorActionPreference=$old;if($code -ne 0){throw "Build failed:$Name;see log"}}
Push-Location $repo
try{
 Generate forward80 "$tests/generate_fft160x192_forward_shader.ps1"
 Native forward80_compile "$VulkanBin/glslangValidator.exe" @('-V','--target-env','vulkan1.2',"$out/forward80/forward160x192.comp",'-o',"$out/forward80/forward160x192.spv")
 Native forward80_validate "$VulkanBin/spirv-val.exe" @('--target-env','vulkan1.2',"$out/forward80/forward160x192.spv")
 Generate inverse80 "$tests/hdrvdp3_fft256/compile_inverse160x192.ps1" @('-VulkanBin',$VulkanBin)
 Generate forward96 "$tests/hdrvdp3_fft256/compile_forward192x160.ps1" @('-VulkanBin',$VulkanBin)
 Generate inverse96 "$tests/hdrvdp3_fft256/compile_inverse192x160.ps1" @('-VulkanBin',$VulkanBin)
 foreach($name in @('mixed_filter_io','mean120','mean120_difference_pack','residual120_extended','general_comparison','pathway_mask_product','base120')){Generate $name "$tests/hdrvdp3_dimensions/compile_$name.ps1" @('-VulkanBin',$VulkanBin)}
 Generate spyr_fma "$tests/hdrvdp3_dimensions/compile_spyr_lowpass.ps1" @('-VulkanBin',$VulkanBin)
 Generate spyr_separate "$tests/hdrvdp3_dimensions/compile_spyr_lowpass.ps1" @('-VulkanBin',$VulkanBin,'-NonFma')
 Generate pool_fma "$tests/hdrvdp3_dimensions/compile_pathway_pool.ps1" @('-VulkanBin',$VulkanBin)
 Generate pool_separate "$tests/hdrvdp3_dimensions/compile_pathway_pool.ps1" @('-VulkanBin',$VulkanBin,'-NonFma')
 Generate power "$tests/generate_power_shaders.ps1"
 Native map_compile "$VulkanBin/glslc.exe" @("$out/power/map.comp",'-o',"$out/map.spv",'--target-env=vulkan1.2')
 Native map_validate "$VulkanBin/spirv-val.exe" @('--target-env','vulkan1.2',"$out/map.spv")
 $configure=@('-S',"$tests/hdrvdp3_models",'-B',"$out/model_build",'-G',$CMakeGenerator);if($CMakeGenerator -like 'Visual Studio*'){$configure+=@('-A','x64')}
 Native model_configure cmake $configure
 Native model_build cmake @('--build',"$out/model_build",'--config','Release','--target','export-sp0-model')
 $modelExe=if(Test-Path "$out/model_build/Release/export-sp0-model.exe"){"$out/model_build/Release/export-sp0-model.exe"}elseif(Test-Path "$out/model_build/export-sp0-model.exe"){"$out/model_build/export-sp0-model.exe"}else{"$out/model_build/export-sp0-model"}
 Native model_export $modelExe @("$out/models")
 $a=[ordered]@{forward160="$out/forward80/forward160x192.spv";inverse160="$out/inverse80/inverse160x192.spv";forward192="$out/forward96/forward192x160.spv";inverse192="$out/inverse96/inverse192x160.spv";filterIO="$out/mixed_filter_io/mixed_filter_io.spv";spyrFMA="$out/spyr_fma/spyr_lowpass.spv";spyrSeparate="$out/spyr_separate/spyr_lowpass.spv";mean="$out/mean120/mean120.spv";difference="$out/mean120_difference_pack/mean120_difference_pack.spv";residual="$out/residual120_extended/residual120_extended.spv";poolFMA="$out/pool_fma/pathway_pool.spv";poolSeparate="$out/pool_separate/pathway_pool.spv";comparisonShader="$out/general_comparison/general_comparison.spv";map="$out/map.spv";product="$out/pathway_mask_product/pathway_mask_product.spv";base120="$out/base120/base120.spv";nativeSPYR="$out/models"}
 # nativeSPYR is the historical packaging field name;here it is C++generated model data.
 $a|ConvertTo-Json|Set-Content "$out/generated_assets.json"
 Native package powershell @('-NoProfile','-ExecutionPolicy','Bypass','-File',"$PSScriptRoot/package_pathway_runtime.ps1",'-AssetsManifest',"$out/generated_assets.json",'-OutputDirectory',"$out/runtime")
 $pins=Get-Content "$PSScriptRoot/REFERENCE_PATHWAY_HASHES.json" -Raw|ConvertFrom-Json
 foreach($pin in $pins){if((Get-FileHash -LiteralPath "$out/runtime/$($pin.file)").Hash -ne $pin.sha256){throw "Regenerated payload differs:$($pin.key);no reproduction PASS"}}
 if($pins.Count -ne 20){throw 'Incomplete payload pins'}
 [ordered]@{assets=20;all_pinned_hashes_identical=$true;native_install_required=$false;previous_exports_required=$false;compiler=((& "$VulkanBin/glslc.exe" --version)-join "`n");scope='80x96/96x80,30ppd/age24/D65/sp0/map13';model='380 Float32 words generated from C++ coefficients'}|ConvertTo-Json|Set-Content "$out/reproduction.json"
 '20 source-generated runtime assets match accepted package SHA256 exactly.'|Set-Content "$out/PASS.txt"
 Write-Output "Source-only runtime PASS:$out/runtime"
}finally{Pop-Location}
