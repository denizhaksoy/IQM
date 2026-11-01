param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$Prefix='C:/Program Files/GNU Octave/Octave-11.3.0/mingw64')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output directory required'}
$out=(New-Item -ItemType Directory $OutputDirectory -Force).FullName
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$env:PATH="$Prefix/bin;$env:PATH"
foreach($pair in @(@('decode_exr.cpp','decode-exr.exe'),@('create_exr_controls.cpp','create-exr-controls.exe'))){
 & "$Prefix/bin/g++.exe" -O2 -std=c++20 -fno-fast-math "-I$Prefix/include" "-I$Prefix/include/OpenEXR" "-I$Prefix/include/Imath" (Join-Path $repo ('tests/upiq/'+$pair[0])) "-L$Prefix/lib" -lOpenEXR -lImath-3_1 -o (Join-Path $out $pair[1])
 if($LASTEXITCODE -ne 0){throw ('EXR build failed: '+$pair[0])}
}
& "$Prefix/bin/g++.exe" --version | Set-Content (Join-Path $out 'compiler.txt')
Write-Host ('Decoder built: '+$out)
