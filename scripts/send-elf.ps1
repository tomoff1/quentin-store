param([Parameter(Mandatory=$true)][string]$ConsoleIP,[int]$Port=9021,[string]$ElfPath='')
$ErrorActionPreference='Stop'
if (-not $ElfPath) {
    $ElfPath=@("$PSScriptRoot/../quentin-store.elf", "$PSScriptRoot/../dist/quentin-store.elf", "$PSScriptRoot/../build/ps5/quentin-store.elf") | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $ElfPath) { throw 'ELF absent. Extraire la distribution ou compiler, puis utiliser -ElfPath.' }
}
$bytes=[System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $ElfPath))
if ($bytes.Length -lt 4 -or $bytes[0] -ne 0x7f -or $bytes[1] -ne 0x45 -or $bytes[2] -ne 0x4c -or $bytes[3] -ne 0x46) { throw 'Fichier ELF invalide' }
$client=[System.Net.Sockets.TcpClient]::new()
try { $client.Connect($ConsoleIP,$Port); $stream=$client.GetStream(); $stream.Write($bytes,0,$bytes.Length); $stream.Flush(); Write-Host 'ELF envoye. Cela ne confirme pas son execution sur PS5.' } finally { $client.Dispose() }
