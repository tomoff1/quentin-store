param([switch]$SkipDependencies)
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (-not $SkipDependencies) { python scripts/bootstrap.py; if ($LASTEXITCODE) { throw 'Dependances: echec' } }
    python scripts/build_ps5.py
    if ($LASTEXITCODE) { throw 'Compilation PS5: echec' }
    python scripts/package.py
    if ($LASTEXITCODE) { throw 'Distribution: echec' }
} finally { Pop-Location }
