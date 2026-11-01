param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$Runtime,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh contract output required'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName;$exe=(Resolve-Path $Executable).Path;$source=(Resolve-Path $Runtime).Path
& $exe $source > "$out/valid.log";if($LASTEXITCODE){throw 'Valid RGB package rejected'}
$cases=@('version','profile','missing','duplicate','unknown','escape','absolute','digest','corrupt','spirv','base_model','base_profile')
foreach($case in $cases){
 $dir="$out/$case";New-Item -ItemType Directory $dir | Out-Null
 Get-ChildItem -LiteralPath $source | Copy-Item -Destination $dir -Recurse
 $lines=[IO.File]::ReadAllLines("$dir/rgb.runtime")
 switch($case){
  'version'{$lines[0]=$lines[0].Replace('RGB 1','RGB 99')}
  'profile'{$lines[0]=$lines[0].Replace('age24','age60')}
  'missing'{$lines=$lines[0..($lines.Length-2)]}
  'duplicate'{$lines+=$lines[2]}
  'unknown'{$lines[2]=$lines[2].Replace('projection ','unknown ')}
  'escape'{$lines[2]=$lines[2].Replace('assets/projection.spv','../projection.spv')}
  'absolute'{$lines[2]=$lines[2].Replace('assets/projection.spv',($source.Replace('\','/')+'/assets/projection.spv'))}
  'digest'{$lines[2]=$lines[2] -replace '[0-9a-f]{64}$',('0'*64)}
  'corrupt'{$file="$dir/assets/projection.spv";$data=[IO.File]::ReadAllBytes($file);$data[16]=$data[16] -bxor 1;[IO.File]::WriteAllBytes($file,$data)}
  'spirv'{$file="$dir/assets/projection.spv";[IO.File]::WriteAllBytes($file,(New-Object byte[] 20));$lines[2]='projection "assets/projection.spv" '+(Get-FileHash $file).Hash.ToLowerInvariant()}
  'base_profile'{$file="$dir/base/pathway.runtime";$text=[IO.File]::ReadAllText($file).Replace('age24','age60');[IO.File]::WriteAllText($file,$text);$lines[1]='base "base/pathway.runtime" '+(Get-FileHash $file).Hash.ToLowerInvariant()}
  'base_model'{
   $model="$dir/base/assets/lo0.txt";[IO.File]::WriteAllText($model,"1 1`n0`n");$hash=(Get-FileHash $model).Hash.ToLowerInvariant()
   $base="$dir/base/pathway.runtime";$baseLines=[IO.File]::ReadAllLines($base);$baseLines=@($baseLines|ForEach-Object{if($_ -like 'lo0 *'){'lo0 "assets/lo0.txt" '+$hash}else{$_}})
   [IO.File]::WriteAllLines($base,$baseLines,[Text.UTF8Encoding]::new($false));$lines[1]='base "base/pathway.runtime" '+(Get-FileHash $base).Hash.ToLowerInvariant()
  }
 }
 [IO.File]::WriteAllLines("$dir/rgb.runtime",$lines,[Text.UTF8Encoding]::new($false))
 & $exe $dir --reject > "$out/$case.log";if($LASTEXITCODE){throw "Malformed RGB package accepted:$case"}
}
Set-Content "$out/PASS.txt" 'Valid RGB runtime accepted;12 malformed packages rejected without GPU;overlay and nested base hashes checked.'
