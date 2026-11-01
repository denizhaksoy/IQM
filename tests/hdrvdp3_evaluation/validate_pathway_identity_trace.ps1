param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$Runtime,[Parameter(Mandatory=$true)][string]$NativeDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$ResultsOnly)
$ErrorActionPreference='Stop';$exe=(Resolve-Path $Executable).Path
if(!$ResultsOnly){if(Test-Path $OutputDirectory){throw 'Fresh trace outputs required'};New-Item -ItemType Directory $OutputDirectory|Out-Null}
$out=(Resolve-Path $OutputDirectory).Path;$records=@()
foreach($pattern in @('asymmetric','impulse','hdr_ramp')){foreach($shape in @('h80_w96','h96_w80')){$tag="${pattern}_$shape"
 if(!$ResultsOnly){$ErrorActionPreference='Continue';& $exe "$NativeDirectory/$pattern/$shape" $Runtime "$out/api_$tag" *> "$out/api_$tag.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw "Identity API failed:$tag"}}
 if(!(Test-Path "$out/api_$tag/PASS.txt")){throw 'Missing identity trace PASS'};if((Get-Content "$out/api_$tag.log" -Raw) -match 'VUID-|Validation Error|\[ERROR\]'){throw 'Vulkan errors'}
 $data=[IO.File]::ReadAllBytes((Resolve-Path "$out/api_$tag/base_noise.f64").Path);if($data.Length -ne 24 -or [BitConverter]::ToUInt64($data,0) -ne 0 -or [BitConverter]::ToUInt64($data,8) -ne 0 -or [BitConverter]::ToUInt64($data,16) -ne [BitConverter]::ToUInt64([BitConverter]::GetBytes([double]::PositiveInfinity),0)){throw 'Actual base-noise bits differ'}
 $records+=[pscustomobject]@{case=$tag;native_P_log_scores_exact=$true;GPU_selected_index=0;base_frequency_bits=0;base_noise='positive infinity';D_psychometric_masked_errors=0;Q=10;Q_JOD=10}
}}
$records|ConvertTo-Json|Set-Content "$out/api_results.json"
Get-FileHash -LiteralPath $exe,$PSCommandPath|Select-Object Path,Hash|ConvertTo-Json|Set-Content "$out/api_hashes.json"
Set-Content "$out/API_PASS.txt" 'Six actual identity API traces:native P/log/Q bits;GPU index0;frequency+0 andnoise+Inf exactbinary64bits;allD/psychometric/masked/quality errors0;Q=Q_JOD=10;no Vulkan errors.'
