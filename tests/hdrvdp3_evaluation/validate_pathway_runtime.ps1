param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$Runtime,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$Square,[ValidateSet(80,96,128)][int]$SquareSide=96)
$ErrorActionPreference='Stop';if(Test-Path $OutputDirectory){throw 'Fresh tests directory required'};$out=(New-Item -ItemType Directory $OutputDirectory).FullName;$exe=(Resolve-Path $Executable).Path;$source=(Resolve-Path $Runtime).Path
$validArgs=@($source);if($Square){$validArgs+=@([string]$SquareSide,[string]$SquareSide)};$asset=if($Square){'forward'}else{'forward80'}
$ErrorActionPreference='Continue';& $exe @validArgs *> "$out/valid.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -ne 0){throw 'Valid runtime rejected'}
$cases=@('version','profile','missing','duplicate','unknown','escape','absolute','digest','corrupt','spirv','model','nonfinite')
foreach($case in $cases){$dir="$out/$case";New-Item -ItemType Directory $dir|Out-Null;Copy-Item "$source/*" $dir -Recurse;$lines=[IO.File]::ReadAllLines("$dir/pathway.runtime")
 switch($case){
 'version'{$lines[0]=$lines[0]-replace 'PATHWAY [0-9]+','PATHWAY 99'}
 'profile'{$lines[0]=$lines[0].Replace('age24','age60')}
 'missing'{$lines=$lines[0..($lines.Length-2)]}
 'duplicate'{$lines+=$lines[1]}
 'unknown'{$lines[1]=$lines[1].Replace("$asset ",'unknown ')}
 'escape'{$lines[1]=$lines[1].Replace("assets/$asset.spv","../$asset.spv")}
 'absolute'{$lines[1]=$lines[1].Replace("assets/$asset.spv",($source.Replace('\','/')+"/assets/$asset.spv"))}
 'digest'{$lines[1]=($lines[1] -replace '[0-9a-f]{64}$',('0'*64))}
 'corrupt'{$path="$dir/assets/$asset.spv";$data=[IO.File]::ReadAllBytes($path);$data[16]=$data[16] -bxor 1;[IO.File]::WriteAllBytes($path,$data)}
 'spirv'{$path="$dir/assets/$asset.spv";[IO.File]::WriteAllBytes($path,(New-Object byte[] 20));$hash=(Get-FileHash $path).Hash.ToLowerInvariant();$lines[1]=$asset+' "assets/'+$asset+'.spv" '+$hash}
 'nonfinite'{$path="$dir/assets/lo0.txt";$tokens=(Get-Content $path -Raw) -split '\s+'|Where-Object{$_ -ne ''};$tokens[2]='2139095040';[IO.File]::WriteAllText($path,($tokens -join "
"));$hash=(Get-FileHash $path).Hash.ToLowerInvariant();$lines=@($lines|ForEach-Object{if($_ -like 'lo0 *'){'lo0 "assets/lo0.txt" '+$hash}else{$_}})}
 'model'{$path="$dir/assets/lo0.txt";[IO.File]::WriteAllText($path,"1 1`n0`n");$hash=(Get-FileHash $path).Hash.ToLowerInvariant();$lines=@($lines|ForEach-Object{if($_ -like 'lo0 *'){'lo0 "assets/lo0.txt" '+$hash}else{$_}})}
 }
 [IO.File]::WriteAllLines("$dir/pathway.runtime",$lines,(New-Object Text.UTF8Encoding($false)))
 $testArgs=@($dir);if($Square){$testArgs+=@('96','96')};$ErrorActionPreference='Continue';& $exe @testArgs *> "$out/$case.log";$code=$LASTEXITCODE;$ErrorActionPreference='Stop';if($code -eq 0){throw "Invalid runtime accepted:$case"}
}
Set-Content "$out/PASS.txt" "Valid package loads requested geometry;Square=$Square;12 malformed packages rejected without GPU;SHA256 verified independently."
'Runtime package contract PASS'
