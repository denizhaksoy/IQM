param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$Runtime,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$Linear)
$ErrorActionPreference='Stop';if(Test-Path $OutputDirectory){throw 'Fresh709 contract directory required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName;$source=(Resolve-Path $Runtime).Path;$exe=(Resolve-Path $Executable).Path
$manifestName=if($Linear){'linear.runtime'}else{'rgb709.runtime'}
& $exe $source > "$out/valid.log";if($LASTEXITCODE){throw 'Valid709 runtime rejected'}
foreach($case in @('version','profile','missing','duplicate','unknown','escape','absolute','digest','corrupt','spirv','native_profile','native_digest')){
 $dir="$out/$case";New-Item -ItemType Directory $dir|Out-Null;Get-ChildItem -LiteralPath $source|Copy-Item -Destination $dir -Recurse
 $lines=[IO.File]::ReadAllLines("$dir/$manifestName")
 switch($case){
 'version'{$lines[0]=$lines[0] -replace ' (1) ',' 99 '}
 'profile'{$lines[0]=$lines[0].Replace('age24','age60')}
 'missing'{$lines=$lines[0..1]}
 'duplicate'{$lines+=$lines[2]}
 'unknown'{$lines[2]=$lines[2].Replace('conversion ','unknown ')}
 'escape'{$lines[2]=$lines[2].Replace('assets/conversion.spv','../conversion.spv')}
 'absolute'{$lines[2]=$lines[2].Replace('assets/conversion.spv',($source.Replace('\','/')+'/assets/conversion.spv'))}
 'digest'{$lines[2]=$lines[2] -replace '[0-9a-f]{64}$',('0'*64)}
 'corrupt'{$file="$dir/assets/conversion.spv";$b=[IO.File]::ReadAllBytes($file);$b[16]=$b[16] -bxor 1;[IO.File]::WriteAllBytes($file,$b)}
 'spirv'{$file="$dir/assets/conversion.spv";[IO.File]::WriteAllBytes($file,(New-Object byte[] 20));$lines[2]='conversion "assets/conversion.spv" '+(Get-FileHash $file).Hash.ToLowerInvariant()}
 'native_profile'{$file="$dir/native/rgb.runtime";[IO.File]::WriteAllText($file,([IO.File]::ReadAllText($file).Replace('age24','age60')));$lines[1]='native "native/rgb.runtime" '+(Get-FileHash $file).Hash.ToLowerInvariant()}
 'native_digest'{$file="$dir/native/assets/projection.spv";$b=[IO.File]::ReadAllBytes($file);$b[16]=$b[16] -bxor 1;[IO.File]::WriteAllBytes($file,$b)}
 }
 [IO.File]::WriteAllLines("$dir/$manifestName",$lines,[Text.UTF8Encoding]::new($false))
 & $exe $dir --reject > "$out/$case.log";if($LASTEXITCODE){throw "Invalid709 runtime accepted:$case"}
}
$kind=if($Linear){'linear-color'}else{'709'}
Set-Content "$out/PASS.txt" "Valid $kind package accepted;12 malformedoverlay/nestednative packages rejected without GPU."
