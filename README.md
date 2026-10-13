# QUENTIN STORE

Application homebrew PS5 native en C++ : catalogue libre, téléchargement HTTPS, reprise et SHA-256. Version **0.1.0 expérimentale**. Deux vrais exécutables ELF sont compilés : `quentin-store.elf` et `quentin-blocks.elf`.

**Statut honnête : compilation PS5 réussie sur Windows ; exécution PS5 13.60 non testée.** La publication distante et le run GitHub Actions sont suivis dans `docs/VALIDATION.md`. Le site désactive son bouton de téléchargement tant que la release publique n'existe pas.

## Démarrage sur PS5

1. Extraire `dist/QUENTIN-STORE-0.1.0.zip`.
2. Via FTP, créer `/data/quentin-store/` et y copier `ca-bundle.crt`, `font.ttf`, `config.json`, `catalog.json`.
3. Avec le jailbreak déjà actif et votre ELF Loader compatible sur 9021, envoyer l'ELF :

```powershell
./scripts/send-elf.ps1 -ConsoleIP 192.168.1.50
```

L'adresse ci-dessus est un exemple à remplacer. Alternative FTP : copier `quentin-store.elf` dans `/data/quentin-store/` puis demander au chargeur de lancer `file:/data/quentin-store/quentin-store.elf` si votre version prend en charge cette commande. Voir [le guide PS5](docs/INSTALLATION-PS5.md).

Les URL par défaut ciblent `tomoff1/quentin-store`. Avant publication de ce dépôt et de la release, elles ne répondront pas. Le catalogue embarqué reste consultable ; Quentin Blocks est également fourni dans l'archive et se lance directement avec ELF Loader.

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

Le succès de compilation ne garantit pas vidéo, manette, réseau ou lancement sur votre environnement 13.60. Aucun PKG, eboot signé, image exFAT, intégration ShadowMountPlus ou installation automatique n'est généré. L'application est un payload ELF graphique, pas encore un titre enregistré dans le menu système PS5. Le rendu logiciel vise une navigation fluide, sans mesure de performance sur console. Les illustrations sont intégrées, pas des images distantes de tiers. Le catalogue initial contient uniquement les deux projets originaux dont les droits de redistribution sont définis ici.

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

WSL n'était pas accessible dans l'environnement de création (`Wsl/E_ACCESSDENIED`) ; la compilation a réellement été faite sur Windows par LLVM. Le workflow GitHub Actions pour Ubuntu est préparé mais n'a pas encore été exécuté sur GitHub.

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
gh auth login
./scripts/publish-github.ps1
```

Cloudflare Pages : dépôt GitHub `tomoff1/quentin-store`, preset « None », commande de build vide, dossier de sortie `site`. GitHub sert les ELF via Releases, Cloudflare sert la présentation. Le catalogue et les mises à jour sont des fichiers JSON publics statiques, donc aucun VPS nécessaire.

## Architecture et licences

`src/core.hpp` : schéma, sécurité et recherche. `src/network.cpp` : TLS, reprise et fichiers. `src/main.cpp` : interface et tâche réseau. `src/blocks.cpp` : jeu original. `scripts/` : dépendances, compilation, distribution, transfert et publication. `site/` : page Cloudflare statique. `tests/` : essais locaux. `.github/workflows/build.yml` : CI.

Code original : GPL-3.0-or-later. Les dépendances conservent leurs propres licences. [Audit des outils](docs/OUTILS-ET-LICENCES.md), `LICENSE` et `third_party/`. Projet indépendant, aucune affiliation Sony ; aucun jeu commercial, ROM, BIOS ou SDK propriétaire fourni.
