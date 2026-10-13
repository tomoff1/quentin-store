# Publication publique à 0 €

Préparée pour `tomoff1/quentin-store`. Aucun compte n'est créé par le projet. Aucune publication distante n'a été effectuée au moment de la livraison locale.

## GitHub

1. Installer GitHub CLI depuis sa source officielle et se connecter avec `gh auth login`. Faire la connexion dans le navigateur ; ne pas communiquer de mot de passe ou token dans le chat.
2. Depuis le projet compilé, exécuter `./scripts/publish-github.ps1`.
3. Le script génère le catalogue et le manifeste avec les tailles et hashes des vrais ELF, crée le dépôt public si absent, pousse `main` et crée la prerelease `v0.1.0`.
4. En cas de dépôt existant différent ou de branche distante non compatible, le push est refusé ; le script ne force pas la réécriture de l'historique. En cas de tag déjà publié, il refuse la nouvelle release ; examiner la version avant de relancer.

URLs prévues, à vérifier après publication :

- Code : https://github.com/tomoff1/quentin-store
- Releases : https://github.com/tomoff1/quentin-store/releases
- Catalogue : https://raw.githubusercontent.com/tomoff1/quentin-store/main/site/catalog.json
- Mises à jour : https://raw.githubusercontent.com/tomoff1/quentin-store/main/site/update.json

## Cloudflare Pages

Dans le compte Cloudflare existant : Workers & Pages → créer une application Pages → importer le dépôt `tomoff1/quentin-store`.

| Réglage | Valeur |
|---|---|
| Branche | main |
| Framework | None |
| Build command | vide |
| Build output directory | site |
| Root directory | racine du dépôt |

Cloudflare attribue un domaine `pages.dev`. Le nom définitif n'est pas inventé à l'avance. Vérifier la page et le bouton après mise en ligne de la release. Alternative CLI gratuite : `npx wrangler pages deploy site --project-name quentin-store` après connexion Cloudflare et création du projet, conformément aux outils disponibles dans votre compte.

Le site vérifie que l'asset demandé existe dans la release avant d'activer le téléchargement. Une panne de l'API ou une limite de débit peut laisser le bouton désactivé ; le lien « GitHub Releases » reste accessible.

## Catalogue et droits

Le catalogue livré référence uniquement QUENTIN STORE et Quentin Blocks, sources complètes sous GPL-3.0-or-later. Pas de mirror d'un homebrew tiers sans vérifier sa licence et les obligations de fourniture du code correspondant. Chaque entrée exige `redistribution: true`, une licence, un lien source HTTPS, une taille réelle et un SHA-256.

GitHub Actions compile et teste les PR et commits. Il charge des artefacts CI, sans publier automatiquement une release ; le script explicite de publication fait cette dernière étape. Les limites des offres gratuites évoluent ; rester dans celles de votre compte et consulter les documentations officielles indiquées dans l'audit.

## Versions suivantes

Modifier la version de façon cohérente dans `src/core.hpp`, les textes UI, `scripts/package.py`, le script de publication et le README. Recompiler, exécuter tous les tests, générer le manifeste, vérifier les hashes, publier les sources et ELF ensemble. La version `0.1.0` initiale n'entraîne volontairement aucune mise à jour disponible sur elle-même.
