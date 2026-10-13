# QUENTIN STORE

Application homebrew PS5 en C++ : catalogue libre, téléchargement HTTPS, reprise et SHA-256. Version **0.1.2 expérimentale** : ELF autonome et **lanceur PKG PS5 natif** (`PPSA99741`). Le lanceur contient le store et l'envoie au chargeur ELF local 9021 ; ce chargeur doit rester actif.

**Statut : compilation PS5 et tests PC réussis ; fonctionnement du PKG natif sur 13.60 à confirmer.** Les anciens PKG PS4 1.00/1.01 sont refusés sur la console de l'utilisateur. La publication distante et les validations sont suivies dans `docs/VALIDATION.md`.

## Démarrage sur PS5

1. Télécharger [QUENTIN-STORE-PS5.pkg](https://github.com/tomoff1/quentin-store/releases/download/v0.1.2/QUENTIN-STORE-PS5.pkg) et l'installer avec un installateur acceptant les PKG PS5.
2. Activer le chargeur ELF 9021, puis ouvrir **QUENTIN STORE PS5**. Le lanceur transmet le store embarqué et se ferme. Le port 9021 n'est pas un installateur PKG.
3. Le store initialise automatiquement ses donnees dans `/data/quentin-store/` : police et ressources sont integrees, aucun fichier annexe a copier par FTP.

Le script optionnel `scripts/send-elf.ps1 -ConsoleIP ADRESSE_IP_PS5 -ElfPath CHEMIN_ELF` utilise le port 9021. Le loader doit etre actif. Voir [le guide PS5](docs/INSTALLATION-PS5.md).

Le catalogue initial contient Quentin Blocks. Les URL par defaut ciblent le catalogue public de `tomoff1/quentin-store` et permettent sa mise a jour.

## Fonctions présentes

- SDL2 natif, rendu logiciel 1280 × 720, thème sombre, illustrations originales dessinées par le programme et animation du focus.
- Accueil, catalogue, recherche au clavier virtuel, catégories, fiches, historique et paramètres.
- Navigation via stick/D-pad, X, Cercle, Carré, Triangle et L1/R1 ; branche PS5 SDL utilisant scePad.
- Catalogue JSON versionné et cache local ; téléchargement unique en arrière-plan, interface active pendant le transfert.
- HTTPS uniquement, validation TLS et des certificats, délais réseau, limite de taille, reprise avec contrôle du `Content-Range`, fallback si le serveur ignore Range.
- Vérification de la taille et du SHA-256 ; fichier `.part` conservé en cas d'interruption, rejeté en cas de hash incorrect.
- Historique de 100 téléchargements vérifiés, diagnostic local sans télémétrie.
- Vérification d'un manifeste de mise à jour, nouveautés, téléchargement avec hash ; remplacement manuel de l'ELF.
- Quentin Blocks : jeu de casse-briques original, code source inclus, GPL-3.0-or-later.

## Limites de la première version

Le succès de compilation ne garantit pas vidéo, manette, réseau ou lancement sur votre environnement 13.60. Le PKG est un lanceur PS5 natif expérimental avec un `eboot.bin` FSELF et `param.json` ; le store graphique reste un payload ELF. Le chargeur local 9021 est nécessaire. Aucun port complet vers le contexte système, image exFAT ou installation automatique n'est fourni. Le rendu logiciel vise une navigation fluide, sans mesure de performance sur console. Les illustrations sont intégrées. Le catalogue contient des projets originaux dont les droits de redistribution sont définis ici.

La police Outfit fournie avec sa licence OFL permet le rendu UTF-8 avec SDL_ttf ; une police bitmap sert de secours si le fichier de police manque. Les hashes proviennent du catalogue HTTPS de confiance, sans signature cryptographique indépendante du manifeste. Un serveur de catalogue compromis pourrait donc remplacer un fichier et son hash.

## Compilation reproductible

Windows 11 : Python 3.10 ou ultérieur est requis. Aucun changement système effectué par bootstrap ; les dépendances sont extraites dans `.deps/`.

```powershell
./scripts/build-windows.ps1
python scripts/package.py --repository tomoff1/quentin-store
```

Le premier téléchargement représente environ 1,3 Go : SDK public v0.44, ports PacBrew v0.41.1, LLVM officiel 18.1.8 pour Windows. Les SHA-256 des trois archives sont verrouillés dans `dependencies.lock.json`. Les sources applicatives sont compilées pour `x86_64-sie-ps5` puis liées avec le script ELF et le CRT du SDK public. Aucun SDK Sony propriétaire ni SPRX déchiffré n'est requis ou inclus.

Linux/WSL Ubuntu 24.04 :

```sh
sudo apt-get update
sudo apt-get install -y clang-18 lld-18 python3 g++ cmake
bash scripts/build-linux.sh
```

WSL n'était pas accessible dans l'environnement de création (`Wsl/E_ACCESSDENIED`) ; la compilation a réellement été faite sur Windows par LLVM. GitHub Actions a également exécuté les tests et la compilation PS5 avec succès sur Ubuntu 24.04.

## Tests

```sh
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Sous Ubuntu, les tests réseau utilisent le même `network.cpp` que l'ELF :

```sh
sudo apt-get install -y libsdl2-dev libsdl2-gfx-dev libsdl2-ttf-dev libcurl4-openssl-dev libssl-dev openssl
cmake -S . -B build/host -DQS_DESKTOP=ON
cmake --build build/host
python3 tests/https_fixture.py --binary build/host/network-tests
```

Le serveur TLS de test reste sur `127.0.0.1` et crée un certificat éphémère explicitement fourni comme autorité de test. La validation TLS du code n'est jamais désactivée. Voir [le rapport réel](docs/VALIDATION.md).

## Publication gratuite

Le code, les ELF et le site sont prêts pour le compte `tomoff1`. Les actions distantes nécessitent votre connexion, pas votre mot de passe dans le chat. Voir [PUBLICATION.md](docs/PUBLICATION.md).

```powershell
winget install GitHub.cli
gh auth login --scopes workflow
./scripts/publish-github.ps1
```

Cloudflare Pages : dépôt GitHub `tomoff1/quentin-store`, preset « None », commande de build vide, dossier de sortie `site`. GitHub sert les ELF via Releases, Cloudflare sert la présentation. Le catalogue et les mises à jour sont des fichiers JSON publics statiques, donc aucun VPS nécessaire.

Livraison publique : [site](https://quentin-store.pages.dev/), [code source](https://github.com/tomoff1/quentin-store), [version téléchargeable](https://github.com/tomoff1/quentin-store/releases/tag/v0.1.2). Cloudflare a été déployé par upload direct ; la connexion automatique au dépôt reste optionnelle. Pour redéployer : `./scripts/publish-cloudflare.ps1`.

## Architecture et licences

`src/core.hpp` : schéma, sécurité et recherche. `src/network.cpp` : TLS, reprise et fichiers. `src/main.cpp` : interface et tâche réseau. `src/blocks.cpp` : jeu original. `scripts/` : dépendances, compilation, distribution, transfert et publication. `site/` : page Cloudflare statique. `tests/` : essais locaux. `.github/workflows/build.yml` : CI.

Code original : GPL-3.0-or-later. Les dépendances conservent leurs propres licences. [Audit des outils](docs/OUTILS-ET-LICENCES.md), `LICENSE` et `third_party/`. Projet indépendant, aucune affiliation Sony ; aucun jeu commercial, ROM, BIOS ou SDK propriétaire fourni.
