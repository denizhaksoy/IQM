param([Parameter(Mandatory=$true)][string]$Spec)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskSpec=Get-Content -LiteralPath $Spec -Raw -Encoding UTF8 | ConvertFrom-Json
$taskWorkspace=[IO.Path]::GetFullPath($taskSpec.workspace).TrimEnd('\')
$taskDestination=[IO.Path]::GetFullPath($taskSpec.destination).TrimEnd('\')
if(-not $taskDestination.StartsWith($taskWorkspace+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Destination outside workspace'}
[IO.Directory]::CreateDirectory($taskDestination) | Out-Null
$taskZip=[IO.Compression.ZipFile]::OpenRead($taskSpec.archive)
$taskRecords=@()
try {
 foreach($taskId in $taskSpec.identifiers){
  if($taskId -notmatch '^[A-Za-z0-9_.-]+$'){throw 'Invalid video identifier'}
  $taskEntry=$taskZip.GetEntry('LIVE_HDR_MP4/'+$taskId+'.mp4')
  if($null -eq $taskEntry){throw ('Archive entry missing: '+$taskId)}
  $taskTarget=[IO.Path]::GetFullPath([IO.Path]::Combine($taskDestination,$taskId+'.mp4'))
  if(-not $taskTarget.StartsWith($taskDestination+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid resolved target'}
  $taskStream=$taskEntry.Open();$taskHash=[Security.Cryptography.SHA256]::Create()
  try{$taskEntryHash=[BitConverter]::ToString($taskHash.ComputeHash($taskStream)).Replace('-','').ToLowerInvariant()}finally{$taskStream.Dispose();$taskHash.Dispose()}
  $taskReused=Test-Path -LiteralPath $taskTarget
  if(-not $taskReused){[IO.Compression.ZipFileExtensions]::ExtractToFile($taskEntry,$taskTarget,$false)}
  $taskActualHash=(Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash.ToLowerInvariant()
  if($taskActualHash -ne $taskEntryHash){throw ('Existing/extracted video differs from archive; preserved: '+$taskId)}
  $taskRecords+=@{video=$taskId;bytes=$taskEntry.Length;sha256=$taskActualHash;reused=$taskReused}
 }
}finally{$taskZip.Dispose()}
$taskArchiveHash=(Get-FileHash -LiteralPath $taskSpec.archive -Algorithm SHA256).Hash.ToLowerInvariant()
$taskJson=@{archive_sha256=$taskArchiveHash;records=$taskRecords} | ConvertTo-Json -Depth 6
[IO.File]::WriteAllText($taskSpec.extraction_report,$taskJson)
