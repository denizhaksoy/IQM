param([Parameter(Mandatory=$true)][string]$ExportDirectory,
 [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$export=(Resolve-Path -LiteralPath $ExportDirectory).Path
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh validation log directory required'}
$out=(New-Item -ItemType Directory -Path $OutputDirectory).FullName
$octave='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe'
$records=@()
foreach($name in @('validate_fft_preprocessing','validate_spyr_spatial','validate_csf_maps','validate_masking_maps','validate_difference_maps','validate_quality_scores')){
 $arguments=@('--quiet','--no-gui',"tests/$name.m",$export)
 if($name -eq 'validate_fft_preprocessing'){$arguments+=@('--strict','--fresh-fft-plans')}
 if($name -eq 'validate_quality_scores'){$arguments+=@('--official','--fresh-fft-plans')}
 $ErrorActionPreference='Continue';& $octave @arguments *> "$out/$name.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop'
 Get-Content "$out/$name.log" -Tail 3
 $records+=[ordered]@{check=$name;exit=$code};$records|ConvertTo-Json|Set-Content "$out/results.json"
 if($code){throw "Exact stage validation failed:$name"}
}
Set-Content "$out/PASS.txt" 'Six exact validators passed with explicit fresh native single FFTW wisdom,3threads/estimate.'
