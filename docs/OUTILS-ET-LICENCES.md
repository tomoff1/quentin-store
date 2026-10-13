# Audit technique — 13 octobre 2026

Sources primaires consultées avant choix du SDK. Les déclarations des projets tiers ne valent pas test de QUENTIN STORE sur la console de l'utilisateur.

| Projet | Utilisation retenue | Licence vérifiée | Compatibilité pertinente |
|---|---|---|---|
| [ps5-payload-sdk](https://github.com/ps5-payload-dev/sdk) v0.44 | CRT, libc, en-têtes et stubs publics ; ELF PIE x86-64 | GPL-3.0-or-later, en-têtes FreeBSD sous licences BSD | SDK pour PS5 exploitées ; README cite plusieurs loaders. Pas de certification de cette application en 13.60 |
| [PacBrew](https://github.com/ps5-payload-dev/pacbrew-repo) v0.41.1 | Bibliothèques PS5 précompilées | Recettes GPL-3.0 ; bibliothèques selon leurs licences | SDL, curl, TLS et C++ disponibles. Une recette SDL indique historiquement LGPL ; le fichier LICENSE.txt du SDL2 réellement consulté indique zlib |
| [SDL PS5](https://github.com/ps5-payload-dev/SDL) | Vidéo PS5 et scePad via backend SDL | zlib | Backend PS5 existant ; ce store ne dépend pas d'offsets kernel spécifiques au firmware |
| [elfldr](https://github.com/ps5-payload-dev/elfldr) | Protocole de transfert ELF sur 9021 | GPL-3.0-or-later | README décrit les payloads ELF et le lancement par chemin `file:` ; environnement jailbreak compatible requis |
| [LLVM](https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8) | clang et lld 18.1.8 | Apache-2.0 WITH LLVM-exception | Compilation croisée réellement effectuée sur Windows |
| [ps5link-sdk](https://github.com/Rufidj/ps5link-sdk) | Voie de titre natif examinée, non utilisée | Pas de conclusion de licence nécessaire à la livraison : aucun code copié | README annonce tests firmware 9.00 et autres firmwares non testés ; signer/packager ajoute une chaîne à vérifier |
| [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) | Examiné pour la distribution ; non intégré | GPL-3.0 | Auto-monteur d'images et titres, dépendant d'un environnement kstuff ; aucun emballage de titre fourni ici |
| [Overdrive releases](https://github.com/half144/overdrive-releases) | Référence d'ergonomie uniquement | Aucun code ou asset repris | Le mainteneur annonce essais 13.60 pour son propre programme. Cela ne certifie pas QUENTIN STORE |

La voie ELF est retenue car le SDK public fournit une chaîne documentée qui a effectivement compilé ici. Un titre de menu PS5 demanderait de valider le linker, la signature homebrew, les métadonnées, l'image et ShadowMountPlus ; ces éléments ne sont pas simulés.

## Bibliothèques liées et obligations

Code du store et jeu original : GPL-3.0-or-later. nlohmann/json 3.11.3 : MIT, licence incluse. SDL2, SDL2_gfx et SDL2_ttf : zlib. curl : licence curl. OpenSSL : Apache-2.0. FreeType : option FreeType License, avec attribution suivante : **Portions of this software are copyright © The FreeType Project (www.freetype.org). All rights reserved.** Libpng : libpng-2.0. zlib : zlib. bzip2 : bzip2-1.0.6. libsamplerate : BSD-2-Clause. libpsl : MIT. zstd : BSD-3-Clause (option retenue). libiconv : LGPL-2.1-or-later. libc++, libc++abi et libunwind : Apache-2.0 WITH LLVM-exception. Outfit : SIL Open Font License 1.1, fichier de licence inclus. Le bundle de CA Mozilla utilisé par curl suit les notices incluses dans son en-tête.

Les notices de tiers sont conservées dans `third_party/`. Les sources du SDK v0.44 et libiconv 1.17 sont incluses dans `third_party/sources/`. Le code complet de QUENTIN STORE et les scripts de reconstruction sont dans l'archive source. Les objets applicatifs livrés dans `dist/relink/` permettent aussi la réédition de liens avec une bibliothèque LGPL modifiée. `scripts/build_ps5.py` documente tous les arguments du linker ; modifier les bibliothèques extraites dans `.deps/ports/.../lib` puis relancer permet la reconstruction. Aucune restriction n'est imposée à la modification ni au débogage.

Provenance : archives GitHub officielles identifiées par version et SHA-256 dans `dependencies.lock.json`. SDK source tag v0.44 ; PacBrew source tag v0.41.1. Les bibliothèques permissives n'exigent pas toutes l'inclusion des sources, mais leurs notices doivent accompagner les exécutables.

## Hébergement

[GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases) fournit les assets ; [Cloudflare Pages pour HTML statique](https://developers.cloudflare.com/pages/framework-guides/deploy-anything/) héberge le dossier `site/`. La publication effective requiert les sessions des comptes. Les builds CI ne sont pas considérés exécutés tant qu'un run GitHub n'a pas eu lieu.
