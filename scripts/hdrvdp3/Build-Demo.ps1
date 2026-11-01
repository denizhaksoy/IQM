param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if(Test-Path -LiteralPath $OutputDirectory){throw 'A fresh package directory is required.'}
$destination=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory $destination -Force | Out-Null
$checkpoint=([IO.File]::ReadAllText((Join-Path $repo 'validation/double96_log_completed_path.txt'))).Trim()
$source=Join-Path $repo $checkpoint
if(-not (Test-Path -LiteralPath (Join-Path $source 'final_audit_PASS.txt'))){throw 'The v10 validation checkpoint is incomplete.'}
# Use the frozen tested executable, not whichever build happens to be current.
Copy-Item -LiteralPath (Join-Path $source 'IQM-log-used.exe') -Destination (Join-Path $destination 'IQM.exe')
New-Item -ItemType Directory (Join-Path $destination 'runtime') | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'accepted_pack/runtime_repro/runtime') -Destination (Join-Path $destination 'runtime/srgb') -Recurse
New-Item -ItemType Directory (Join-Path $destination 'spectral') | Out-Null
Get-ChildItem (Join-Path $source 'accepted_pack/runtime_repro/runtime/model') -File | Copy-Item -Destination (Join-Path $destination 'spectral')
$invariant=[Globalization.CultureInfo]::InvariantCulture
$cases=@();$lineage=@()
Add-Type -AssemblyName System.Drawing
function Write-Preview([string]$File,[string]$Target,[bool]$HDR){
    $bytes=[IO.File]::ReadAllBytes($File);if($bytes.Length -ne 27648*4){throw 'Unexpected preview input shape'}
    $bitmap=[Drawing.Bitmap]::new(96,96)
    try{for($y=0;$y -lt 96;$y++){for($x=0;$x -lt 96;$x++){$channels=@();for($c=0;$c -lt 3;$c++){$v=[double][BitConverter]::ToSingle($bytes,4*($c*9216+$y*96+$x));if($HDR){$v=[Math]::Pow([Math]::Max(0.0,$v)/(1.0+[Math]::Max(0.0,$v)),1.0/2.2)};$channels+=[int][Math]::Round(255.0*[Math]::Min(1.0,[Math]::Max(0.0,$v)))};$bitmap.SetPixel($x,$y,[Drawing.Color]::FromArgb($channels[0],$channels[1],$channels[2]))}};$bitmap.Save($Target,[Drawing.Imaging.ImageFormat]::Png)}finally{$bitmap.Dispose()}
}
function Add-Case([string]$Id,[string]$Title,[string]$CaseSource,[string]$Export,[string]$Encoding,[string]$Runtime,[string]$Note){
    $dir=Join-Path $destination ('examples/'+$Id);New-Item -ItemType Directory $dir -Force | Out-Null
    foreach($side in @('reference','test')){Copy-Item -LiteralPath (Join-Path $CaseSource ($side+'.pfm')) -Destination (Join-Path $dir ($side+'.pfm'));Write-Preview (Join-Path $Export ($side+'_rgb.f32')) (Join-Path $dir ($side+'.png')) ($Encoding -eq 'rgb-native')}
    $tokens=([IO.File]::ReadAllText((Join-Path $Export 'quality.txt'))).Trim() -split '\s+'
    if($tokens.Count -ne 8){throw 'Unexpected reference scores'}
    $goldens=@(Get-ChildItem $Export -Filter '*.f32' -File | ForEach-Object {[PSCustomObject]@{name=$_.Name;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
    # Keep the original 17-digit strings: Windows PowerShell's JSON numeric
    # serialization can otherwise round a Double before it is read back.
    $script:cases+=[PSCustomObject]@{id=$Id;title=$Title;encoding=$Encoding;runtime=$Runtime;reference=('examples/'+$Id+'/reference.pfm');test=('examples/'+$Id+'/test.pfm');expected=@($tokens);expectedExports=$goldens;note=$Note}
    $script:lineage+=[PSCustomObject]@{id=$Id;source=$CaseSource;referenceExport=$Export}
}
foreach($id in @('stress','ramps','identity')){$dir=Join-Path $source ('public_v10/'+$id+'_oled');Add-Case $id ('sRGB: '+$id) $dir (Join-Path $dir 'output') 'srgb-display' 'runtime/srgb' 'Encoded sRGB inputs, display model applied by the metric. Synthetic fixture; no resizing.'}
$main=([IO.File]::ReadAllText((Join-Path $repo 'validation/main_rgb_completed_path.txt'))).Trim()
$real=Join-Path $repo ($main+'/real/real');$command=Get-Content (Join-Path $real 'command.json') -Raw | ConvertFrom-Json
$runtimeIndex=[array]::IndexOf($command.args,'--hdrvdp3-runtime');$nativeRuntime=[string]$command.args[$runtimeIndex+1]
if(-not [IO.Path]::IsPathRooted($nativeRuntime)){$nativeRuntime=Join-Path $repo $nativeRuntime}
Copy-Item -LiteralPath $nativeRuntime -Destination (Join-Path $destination 'runtime/native') -Recurse
Add-Case 'hdr-photo' 'HDR photograph: local color distortion' $real (Join-Path $real 'export') 'rgb-native' 'runtime/native' 'Memorial Church HDR-derived crop, interpreted as native display RGB. Virtual luminance scaling and local red/blue changes; not an absolute capture calibration. Preview uses v/(1+v) and gamma 1/2.2 only for display.'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Run-Demo.ps1') -Destination (Join-Path $destination 'Run-Demo.ps1')
[PSCustomObject]@{format='IQM_HDRVDP3_DEMO_1';cases=$cases} | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $destination 'demo.json')
$lineage | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $destination 'LINEAGE.json')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'DEMO_README.md') -Destination (Join-Path $destination 'README.md')
Copy-Item -LiteralPath (Join-Path $repo 'tests/DOUBLE96_GPU_LOG_VALIDATION.md') -Destination (Join-Path $destination 'SCOPE.md')
Copy-Item -LiteralPath (Join-Path $repo 'tests/RGB_CONTENT_VALIDATION.md') -Destination (Join-Path $destination 'HDR_PROVENANCE.md')
$hashes=@(Get-ChildItem $destination -Recurse -File | ForEach-Object {[PSCustomObject]@{path=$_.FullName.Substring($destination.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
$hashes | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $destination 'package_hashes.json')
Write-Host ('Demo package: '+$destination)
