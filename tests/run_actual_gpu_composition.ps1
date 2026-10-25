param([Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][string]$AssetsManifest,
 [string]$Octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
Push-Location $repo
try {
 $assets=Get-Content -LiteralPath $AssetsManifest -Raw|ConvertFrom-Json
 foreach($property in $assets.PSObject.Properties){if($property.Name -eq 'jndMode'){if($property.Value -ne '--cpu-lut'){throw 'Unknown JND mode'};continue};if(!(Test-Path -LiteralPath $property.Value)){throw "Missing asset $($property.Name):$($property.Value)"}}
 if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
 New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
 $output=(Resolve-Path -LiteralPath $OutputDirectory).Path
 function Run-Checked([string]$Name,[string]$Exe,[string[]]$Arguments){
  $old=$ErrorActionPreference;$ErrorActionPreference='Continue'
  & $Exe @Arguments *> "$output/$Name.log"
  $code=$LASTEXITCODE;$ErrorActionPreference=$old
  if($code -ne 0){throw "$Name failed, exit=$code; see $output/$Name.log"}
  Write-Output "$Name PASS"
 }
 $filterRoot="$output/filters";New-Item -ItemType Directory $filterRoot|Out-Null
 foreach($shape in @(@(80,96),@(96,80))){$h=$shape[0];$w=$shape[1];$forward=if($h -eq 80){$assets.forward160}else{$assets.forward192};$inverse=if($h -eq 80){$assets.inverse160}else{$assets.inverse192}
  foreach($label in @('ref','test')){$tag="gpu_h${h}_w${w}_mtf_$label";Run-Checked $tag $assets.filterChecker @("$($assets.nativeFilters)/h${h}_w${w}","$h","$w",'mtf',$label,$forward,$inverse,$assets.filterIO,"$filterRoot/$tag")}
 }
 $prepareMode=if($assets.jndMode -eq '--cpu-lut'){'--prepare-only-cpu'}else{'--prepare-only'}
 Run-Checked 'prepare' $assets.jndChecker @($assets.map,$assets.visual,"$output/prepare",$prepareMode,$filterRoot)
 foreach($shape in @(@(80,96),@(96,80))){$h=$shape[0];$w=$shape[1];$forward=if($h -eq 80){$assets.forward160}else{$assets.forward192};$inverse=if($h -eq 80){$assets.inverse160}else{$assets.inverse192}
  foreach($label in @('ref','test')){$long=if($label -eq 'ref'){'reference'}else{'test'};$tag="gpu_h${h}_w${w}_la_${label}_from_gpu";Run-Checked $tag $assets.filterChecker @("$($assets.nativeFilters)/h${h}_w${w}","$h","$w",'la',$label,$forward,$inverse,$assets.filterIO,"$filterRoot/$tag","$output/prepare/h${h}_w${w}_${long}_adaptation_input.f32")}
 }
 $jndArguments=@($assets.map,$assets.visual,"$output/jnd",'--gpu-pupil',$filterRoot,$filterRoot)
 if($assets.jndMode -eq '--cpu-lut'){$jndArguments+=@('--cpu-lut')}
 Run-Checked 'jnd' $assets.jndChecker $jndArguments
 Run-Checked 'serialize_visual' $Octave @('--quiet','--no-gui','tests/prepare_gpu_visual_composition.m',"$output/inputs","$output/jnd",$assets.nativeSPYR,$assets.nativeTrace)
 Run-Checked 'spyr' $assets.spyrChecker @($assets.spyrFMA,$assets.spyrSeparate,"$output/inputs/spyr_inputs","$output/spyr",'--all-chain',$assets.mean,$assets.difference,$assets.residual,$assets.residualGoldens)
 Run-Checked 'serialize_bands' $Octave @('--quiet','--no-gui','tests/prepare_gpu_band_pool_inputs.m',"$output/bands","$output/spyr",$assets.nativeTrace)
 Run-Checked 'pool' $assets.poolChecker @($assets.poolFMA,$assets.poolSeparate,"$output/bands/pool.txt","$output/pool")
 Run-Checked 'comparison' $assets.comparisonChecker @($assets.comparisonShader,$assets.comparisonNative,"$output/comparison")
 Run-Checked 'quality' $assets.maskingChecker @($assets.map,$assets.nativeMasking,"$output/quality",$assets.product,"$output/comparison","$output/bands","$output/pool","$output/inputs/log_inputs",$assets.base120,"$output/spyr")
 Run-Checked 'validate' $Octave @('--quiet','--no-gui','tests/validate_actual_gpu_composition.m',"$output/validated",$assets.nativeFilters,$assets.comparisonNative,"$output/quality",$assets.nativeTrace,$assets.visual)
 $assets|ConvertTo-Json -Depth 5|Set-Content "$output/assets.json"
 Get-Content "$output/validated/PASS.txt"|Set-Content "$output/PASS.txt"
 Write-Output "Experimental composition PASS: $output"
} finally {Pop-Location}
