param([string]$OutputDirectory,[switch]$EnableValidation,[switch]$OpenReport)
$ErrorActionPreference='Stop'
$package=$PSScriptRoot
if(-not $OutputDirectory){$OutputDirectory=Join-Path $package ('results/run_'+[DateTime]::UtcNow.Ticks)}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $OutputDirectory){throw 'A fresh results directory is required.'}
New-Item -ItemType Directory $OutputDirectory -Force | Out-Null
$manifest=Get-Content (Join-Path $package 'demo.json') -Raw | ConvertFrom-Json
$invariant=[Globalization.CultureInfo]::InvariantCulture
$records=@();$cards=@()
foreach($case in $manifest.cases){
    $caseOutput=Join-Path $OutputDirectory $case.id
    $metricArgs=@('--method','HDRVDP3','--hdrvdp3-encoding',$case.encoding,'--hdrvdp3-display','oled','--hdrvdp3-runtime',(Join-Path $package $case.runtime),'--ref',(Join-Path $package $case.reference),'--input',(Join-Path $package $case.test),'--hdrvdp3-export',$caseOutput)
    if($case.encoding -eq 'rgb-native'){$metricArgs+=@('--hdrvdp3-spectral-data',(Join-Path $package 'spectral'))}
    if($EnableValidation){$metricArgs+='--enable-validation'}
    Write-Host ('Running '+$case.title+' ...')
    [PSCustomObject]@{executable='IQM.exe';arguments=$metricArgs} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $OutputDirectory ($case.id+'_command.json'))
    # Capture stderr without PowerShell treating a native diagnostic as a script exception.
    $start=[Diagnostics.ProcessStartInfo]::new()
    $start.FileName=Join-Path $package 'IQM.exe'
    # All generated arguments are paths or fixed options; quote Windows paths explicitly.
    foreach($a in $metricArgs){if($a.Contains('"') -or $a.EndsWith('\')){throw 'Unsupported quote/trailing separator in argument'}}
    $start.Arguments=($metricArgs | ForEach-Object {'"'+$_+'"'}) -join ' '
    $start.UseShellExecute=$false;$start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
    $process=[Diagnostics.Process]::new();$process.StartInfo=$start
    $timer=[Diagnostics.Stopwatch]::StartNew();[void]$process.Start()
    $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
    $process.WaitForExit();$timer.Stop();$log=$stdout.Result+$stderr.Result
    [IO.File]::WriteAllText((Join-Path $OutputDirectory ($case.id+'.log')),$log)
    if($process.ExitCode -ne 0 -or $log -match 'VUID-|\[ERROR\]'){throw ('Metric failed: '+$case.id+'. See saved log.')}
    $tokens=([IO.File]::ReadAllText((Join-Path $caseOutput 'quality.txt'))).Trim() -split '\s+'
    if($tokens.Count -ne 8){throw 'Unexpected quality export shape'}
    $values=@($tokens | ForEach-Object {[double]::Parse($_,$invariant)})
    for($i=0;$i -lt 8;$i++){if([BitConverter]::DoubleToInt64Bits($values[$i]) -ne [BitConverter]::DoubleToInt64Bits([double]::Parse([string]$case.expected[$i],$invariant))){throw ('Reference mismatch: '+$case.id+' value '+$i)}}
    foreach($file in $case.expectedExports){$actual=(Get-FileHash -LiteralPath (Join-Path $caseOutput $file.name) -Algorithm SHA256).Hash;if($actual -ne $file.sha256){throw ('Export mismatch: '+$case.id+'/'+$file.name)}}
    $q=$values[6].ToString('G17',$invariant);$jod=$values[7].ToString('G17',$invariant)
    $records+=[PSCustomObject]@{id=$case.id;encoding=$case.encoding;Q=$values[6];Q_JOD=$values[7];exactReference=$true;comparedQualityValues=8;exactBinaryExports=$case.expectedExports.Count;seconds=$timer.Elapsed.TotalSeconds}
    $cards+='<article><h2>'+[Net.WebUtility]::HtmlEncode($case.title)+'</h2><p>'+[Net.WebUtility]::HtmlEncode($case.note)+'</p><div class="images"><figure><img src="'+$case.id+'_reference.png"><figcaption>Reference preview</figcaption></figure><figure><img src="'+$case.id+'_test.png"><figcaption>Test preview</figcaption></figure></div><p><b>Q: '+$q+'</b> &nbsp; Q_JOD: '+$jod+'</p><p class="pass">All 8 quality values and '+$case.expectedExports.Count+' binary exports match the preserved reference results exactly.</p></article>'
    foreach($side in @('reference','test')){Copy-Item -LiteralPath (Join-Path $package ('examples/'+$case.id+'/'+$side+'.png')) -Destination (Join-Path $OutputDirectory ($case.id+'_'+$side+'.png'))}
    Write-Host ($case.id+': Q='+$q+'; Q_JOD='+$jod+'; exact reference match')
}
$records | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $OutputDirectory 'results.json')
$html=@'
<!doctype html><html lang="en"><meta charset="utf-8"><title>HDR-VDP-3 demonstration</title><style>body{font:17px system-ui;max-width:1050px;margin:40px auto;padding:0 20px;color:#192936;background:#f2f5f7}h1{font-size:32px}article{background:white;padding:24px;margin:24px 0;border-radius:12px}.images{display:flex;gap:24px}figure{margin:0}img{width:192px;height:192px;image-rendering:auto}figcaption{font-size:14px}.pass{color:#176331}aside{padding:20px;background:#e2eaf1}a{color:#176080}</style><h1>HDR-VDP-3: reproducible demonstration</h1><p>C++ / Vulkan implementation, compared with preserved official HDR-VDP-3 v3.0.7 results.</p><aside><b>Validated scope:</b> 96x96, 30 pixels/degree, observer age 24, OLED spectrum, quality/default sp0. Hybrid CPU/GPU implementation. These examples do not establish all-input, all-size or all-device equivalence. Execution time includes startup, transfers and exports; it is not a GPU benchmark. HDR previews are display-only mappings and are never metric inputs.</aside>
'@
    $html+=($cards -join "`n")+'<p>See README.md and SCOPE.md in the package for provenance, requirements and remaining work.</p></html>'
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'report.html'),$html)
Set-Content (Join-Path $OutputDirectory 'PASS.txt') 'PASS:4 demonstration cases;32 quality values exact;all expected binary exports exact.'
Write-Host ('Report: '+(Join-Path $OutputDirectory 'report.html'))
if($OpenReport){Start-Process -FilePath (Join-Path $OutputDirectory 'report.html')}
