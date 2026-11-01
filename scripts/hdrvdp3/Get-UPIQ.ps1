param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$ArchivePath,[ValidateRange(1,86400)][int]$MaxSeconds=3600)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$lock=Get-Content (Join-Path $repo 'tests/upiq/dataset.lock.json') -Raw | ConvertFrom-Json
if(Test-Path -LiteralPath $OutputDirectory){throw 'A fresh acquisition directory is required.'}
$out=(New-Item -ItemType Directory $OutputDirectory -Force).FullName
Copy-Item -LiteralPath (Join-Path $repo 'tests/upiq/dataset.lock.json') -Destination (Join-Path $out 'dataset.lock.json')
$archive=Join-Path $out 'upiq_dataset.zip'
if($ArchivePath){
    if((Get-Item -LiteralPath $ArchivePath).Length -ne $lock.archive.bytes -or (Get-FileHash -LiteralPath $ArchivePath -Algorithm MD5).Hash.ToLowerInvariant() -ne $lock.archive.md5){throw 'Supplied archive differs from the official pinned dataset.'}
    Copy-Item -LiteralPath $ArchivePath -Destination $archive
}else{
    $partial=Join-Path $out 'upiq_dataset.zip.partial'
    # Preserve HTTP error bodies under .partial; never call them a valid archive.
    $status=& curl.exe -L --silent --show-error --connect-timeout 20 --max-time $MaxSeconds --dump-header (Join-Path $out 'download.headers.txt') --output $partial --write-out '%{http_code}' $lock.archive.url
    $curlExit=$LASTEXITCODE
    if($curlExit -ne 0 -or $status -ne '200'){
        [PSCustomObject]@{dataset=$lock.doi;downloaded=$false;validated=$false;curlExit=$curlExit;httpStatus=[string]$status;reason='Official content download failed; preserved response is not a dataset.'} | ConvertTo-Json | Set-Content (Join-Path $out 'status.json')
        Write-Host ('UPIQ download unavailable: HTTP '+$status+', curl '+$curlExit)
        exit 2
    }
    if((Get-Item -LiteralPath $partial).Length -ne $lock.archive.bytes -or (Get-FileHash -LiteralPath $partial -Algorithm MD5).Hash.ToLowerInvariant() -ne $lock.archive.md5){throw 'Archive size/checksum does not match official metadata; partial file preserved.'}
    Rename-Item -LiteralPath $partial -NewName 'upiq_dataset.zip'
}
[PSCustomObject]@{dataset=$lock.doi;downloaded=$true;validated=$true;bytes=(Get-Item -LiteralPath $archive).Length;md5=$lock.archive.md5;sha256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash;extracted=$false} | ConvertTo-Json | Set-Content (Join-Path $out 'status.json')
Write-Host ('Verified original UPIQ archive: '+$archive)
