param([Parameter(Mandatory=$true)][string]$ConsoleIP,[int]$Port=9021,[string]$ElfPath="$PSScriptRoot/../build/ps5/quentin-store.elf")
$ErrorActionPreference='Stop'
$bytes=[System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $ElfPath))
if ($bytes.Length -lt 4 -or $bytes[0] -ne 0x7f -or $bytes[1] -ne 0x45 -or $bytes[2] -ne 0x4c -or $bytes[3] -ne 0x46) { throw 'Fichier ELF invalide' }
$client=[System.Net.Sockets.TcpClient]::new()
try { $client.Connect($ConsoleIP,$Port); $stream=$client.GetStream(); $stream.Write($bytes,0,$bytes.Length); $stream.Flush(); Write-Host 'ELF envoye. Cela ne confirme pas son execution sur PS5.' } finally { $client.Dispose() }
