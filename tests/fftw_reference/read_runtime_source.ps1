function Read-IQMFFTWSource([string]$Directory,[string]$Name) {
    $manifest=Get-Content (Join-Path $Directory 'SOURCES.json') -Raw|ConvertFrom-Json
    $entry=@($manifest.files|Where-Object {$_.file -eq $Name})
    if($entry.Count -ne 1){throw "Missing FFTW source manifest entry:$Name"}
    $text=[IO.File]::ReadAllText((Join-Path $Directory $Name)).Replace("`r`n","`n")
    $sha=[Security.Cryptography.SHA256]::Create()
    try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))).Replace('-','')}
    finally{$sha.Dispose()}
    if($hash -ne $entry[0].sha256_lf){throw "FFTW source hash mismatch:$Name"}
    return $text
}
