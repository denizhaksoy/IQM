param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin=$env:VULKAN_SDK+'/Bin',[string]$Gcc='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/gcc.exe')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory|Out-Null
$tests=Split-Path $PSScriptRoot -Parent
$env:PATH="$(Split-Path $Gcc);$env:PATH"
& $Gcc -O2 -ffp-contract=off "$tests/prepare_double192_twiddles.c" -o "$OutputDirectory/prepare-twiddles.exe"
if($LASTEXITCODE -ne 0){throw 'Twiddle helper compile failed'}
foreach($mode in @('inverse','complex')){
 & "$OutputDirectory/prepare-twiddles.exe" "$OutputDirectory/$mode-twiddles.f64" $mode
 if($LASTEXITCODE -ne 0){throw 'Twiddle preparation failed'}
}
& "$tests/generate_fft192_double_inverse_axis_shader.ps1" -OutputDirectory "$OutputDirectory/first" -TwiddleFile "$OutputDirectory/inverse-twiddles.f64"
& "$tests/generate_complex192_double_inverse_shader.ps1" -OutputDirectory "$OutputDirectory/second" -TwiddleFile "$OutputDirectory/complex-twiddles.f64"
foreach($spec in @(@('first','inverse192_double_axis',32),@('second','complex192_double_inverse',16))){
 $dir="$OutputDirectory/$($spec[0])";$name=$spec[1];$local=$spec[2]
 & "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 "$dir/$name.comp" -o "$dir/default.spv"
 if($LASTEXITCODE -ne 0){throw 'Shader compile failed'}
 & "$VulkanBin/spirv-dis.exe" "$dir/default.spv" -o "$dir/default.spvasm"
 if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
 $s=[IO.File]::ReadAllText("$dir/default.spvasm")
 $entry="OpExecutionMode %main LocalSize $local 1 1"
 if(!$s.Contains($entry)){throw 'Unexpected entrypoint'}
 $s=$s.Replace('OpCapability Shader',"OpCapability Shader`n               OpCapability RoundingModeRTE`n               OpCapability SignedZeroInfNanPreserve")
 $s=$s.Replace($entry,"$entry`n               OpExecutionMode %main RoundingModeRTE 64`n               OpExecutionMode %main SignedZeroInfNanPreserve 64")
 [IO.File]::WriteAllText("$dir/$name.spvasm",$s,[Text.UTF8Encoding]::new($false))
 & "$VulkanBin/spirv-as.exe" --target-env vulkan1.2 "$dir/$name.spvasm" -o "$dir/$name.spv"
 if($LASTEXITCODE -ne 0){throw 'Assembly failed'}
 & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$dir/$name.spv"
 if($LASTEXITCODE -ne 0){throw 'Validation failed'}
}
Copy-Item "$tests/fftw_reference/COPYING" "$OutputDirectory/COPYING"
Get-ChildItem $OutputDirectory -Recurse -File|Get-FileHash|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$OutputDirectory/sha256.json"
