param([switch]$Login,[string]$ProjectName='quentin-store')
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (-not (Get-Command node -ErrorAction SilentlyContinue)) { throw 'Installer Node.js LTS depuis nodejs.org' }
    npm.cmd install --prefix .deps/cloudflare wrangler@4.149.0 --no-audit --no-fund
    if ($LASTEXITCODE) { throw 'Installation de Wrangler impossible' }
    $cli='.deps/cloudflare/node_modules/wrangler/bin/wrangler.js'
    if ($Login) {
        node $cli login --device --scopes account:read user:read pages:write --browser=false
        if ($LASTEXITCODE) { throw 'Connexion Cloudflare requise' }
    }
    node $cli pages deploy site --project-name $ProjectName --branch main --commit-dirty=true
    if ($LASTEXITCODE) { throw 'Deploiement Cloudflare impossible' }
} finally { Pop-Location }
