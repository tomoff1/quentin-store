param([string]$Repository='tomoff1/quentin-store')
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'Installer GitHub CLI: winget install GitHub.cli, puis gh auth login' }
    gh auth status
    if ($LASTEXITCODE) { throw 'Connexion GitHub requise: gh auth login' }
    python scripts/package.py --repository $Repository
    if ($LASTEXITCODE) { throw 'Compiler les ELF avant de publier' }
    if (-not (Test-Path -LiteralPath '.git')) { git init; git branch -M main }
    git add src tests scripts site docs third_party .github CMakeLists.txt README.md LICENSE dependencies.lock.json ca-bundle.crt font.ttf .gitignore
    git diff --cached --quiet
    if ($LASTEXITCODE -eq 1) { git -c user.name='tomoff1' -c user.email='tomoff1@users.noreply.github.com' commit -m 'QUENTIN STORE 0.1.0 experimental' }
    gh repo view $Repository --json name 2>$null
    if ($LASTEXITCODE) { gh repo create $Repository --public --source . --remote origin }
    if ($LASTEXITCODE) { throw 'Creation du depot impossible' }
    $remote=git remote get-url origin
    if ($remote -ne "https://github.com/$Repository.git" -and $remote -ne "git@github.com:$Repository.git") { throw 'Le remote origin ne correspond pas au depot attendu' }
    python scripts/push-github.py
    if ($LASTEXITCODE) { throw 'Push refuse; ne pas forcer, verifier le depot' }
    $files=Get-ChildItem -LiteralPath dist -File | Where-Object { $_.Extension -in '.elf','.zip' -or $_.Name -eq 'SHA256SUMS' } | ForEach-Object FullName
    gh release create v0.1.0 @files --repo $Repository --title 'QUENTIN STORE 0.1.0 — experimental' --notes-file docs/RELEASE.md --prerelease
    if ($LASTEXITCODE) { throw 'Creation de release impossible ou version deja existante' }
} finally { Pop-Location }
