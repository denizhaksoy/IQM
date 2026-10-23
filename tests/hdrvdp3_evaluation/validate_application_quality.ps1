param([Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [switch]$CLI,[switch]$RepresentativeOnly)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath $Executable).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh application validation directory required'}
$root=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$workspace=(Get-Location).Path
$assets="$workspace/validation/asset_generation_639281159480285385/assets_verified"
$prior128="$workspace/validation/map128_api_639281996762386676"
$maps="$workspace/validation/rectangular_maps_639282910800030657"
$stress="$workspace/validation/rectangular_stress_639282921084448635"
$rectangle="$workspace/validation/rectangular_public_639282926432216545/runtime"
$cases=@()
foreach($side in @(32,64,128)){
 if($side -eq 128){$ref="$prior128/native_quality128_localized/reference.pfm";$test="$prior128/native_quality128_localized/test.pfm";$gold="$prior128/public_release/full128_localized";$runtime="$prior128/runtime128"}
 else{$ref="$assets/input/reference$side.pfm";$test="$assets/input/test${side}_localized.pfm";$gold="$assets/golden$side";$runtime=$assets}
 $cases+=[pscustomobject]@{name="square$side";reference=$ref;test=$test;golden=$gold;runtime=$runtime;expected=$(if($side -eq 32){96}else{107})}
}
foreach($shape in @('h64_w128','h128_w64')){foreach($pattern in @('asymmetric_localized','impulse_localized','hdr_ramp_localized','asymmetric_identity')){
 if($RepresentativeOnly -and $pattern -ne 'asymmetric_localized'){continue}
 $name="${shape}_$pattern"
 if($pattern -eq 'asymmetric_localized'){$native="$maps/native_quality_$shape";$gold="$maps/full_fixed_$shape"}
 else{$native="$stress/native_$name";$gold="$stress/gpu_$name"}
 $cases+=[pscustomobject]@{name=$name;reference="$native/reference.pfm";test="$native/test.pfm";golden=$gold;runtime=$rectangle;expected=107}
}}
$records=@()
foreach($case in $cases){
 $env:IQM_FFT_SIZE_FIXTURE='';$env:IQM_SPYR_DIMENSION_PROBE=''
 $env:IQM_LUMINANCE_REFERENCE=$case.reference;$env:IQM_LUMINANCE_TEST=$case.test
 $env:IQM_QUALITY_RUNTIME=$case.runtime;$env:IQM_SPECTRAL_DATA="$assets/spectral";$env:IQM_SPECTRAL_SAMPLES=''
 $env:IQM_PIX_PER_DEGREE='30';$env:IQM_OBSERVER_AGE='24'
 # File dimensions and explicit runtime must override stale synthetic settings.
 $env:IQM_SPYR_SIZE='invalid_synthetic_setting';$env:IQM_FFT_BACKEND='invalid_legacy_setting'
 $arguments=@('--method','HDRVDP3','--enable-validation')
 if($CLI){
  $arguments+=@('--input',$case.test,'--ref',$case.reference,'--hdrvdp3-runtime',$case.runtime,'--hdrvdp3-spectral-data',"$assets/spectral",'--hdrvdp3-export',"$root/export_$($case.name)")
  $env:IQM_LUMINANCE_REFERENCE='invalid_environment';$env:IQM_LUMINANCE_TEST='invalid_environment'
  $env:IQM_QUALITY_RUNTIME='invalid_environment';$env:IQM_SPECTRAL_DATA='invalid_environment'
  $env:IQM_PIX_PER_DEGREE='invalid_environment';$env:IQM_OBSERVER_AGE='invalid_environment'
 }
 $ErrorActionPreference='Continue';& $exe @arguments *> "$root/$($case.name).log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 $log=[IO.File]::ReadAllText("$root/$($case.name).log")
 if($code -or $log -match 'VUID-|Validation Error|\[ERROR\]|ERROR in'){throw "Application quality failed:$($case.name)"}
 $match=[regex]::Match($log,'Quality export directory: ([^\r\n]+)')
 if(!$match.Success){throw 'Missing application export path'}
 $export=$match.Groups[1].Value.Trim();$same=0
 foreach($file in Get-ChildItem -LiteralPath $export -Filter '*.txt'){
  if($file.Name -in @('input_source.txt','pattern.txt','test_transform.txt','map_shader.txt','base_fft_shader.txt','fft_math_shader_dir.txt')){continue}
  if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath "$($case.golden)/$($file.Name)").Hash){throw "Changed application export:$($case.name)/$($file.Name)"}
  ++$same
 }
 if($same -ne $case.expected){throw 'Incomplete application export'}
 $records+=[pscustomobject][ordered]@{case=$case.name;export=$export;numeric_export_hashes_equal=$same;quality_file_bitwise_equal=$true;runtime=$case.runtime}
 $records|ConvertTo-Json|Set-Content "$root/results.json"
 Write-Output "Application exact:$($case.name),$same exports"
}
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$root/hashes.json"
$total=($records|Measure-Object numeric_export_hashes_equal -Sum).Sum
Set-Content "$root/PASS.txt" "$($records.Count) application PFM pairs preserve$total numerical exports and exact quality files;CLI=$CLI."
