# Création du paquet PS5 natif

Ce dossier contient les sources nécessaires pour fabriquer un véritable conteneur PS5 `.pkg` au format FIH debug, avec métadonnées `param.json`. Le paquet contient un lanceur PS5 natif et l'ELF QUENTIN STORE dans `assets/quentin-store.payload`.

Le lanceur transmet l'ELF au chargeur local sur `127.0.0.1:9021`. Le paquet reste expérimental : les contrôles PC ne confirment ni l'installation ni le lancement sur une PS5 physique, notamment en firmware 13.60. L'envoi de l'ELF ne confirme pas son exécution.

## Prérequis

- SDK .NET 10 ou plus récent, avec C# 14.
- Le dossier natif complet, produit par `scripts/build_native_ps5.py`, contenant `eboot.bin`, `sce_module/libc.prx`, `sce_sys/param.json`, `sce_sys/icon0.png` et `assets/quentin-store.payload`.
- Les versions `sdkVersion` et `requiredSystemSoftwareVersion` dans `param.json` doivent valoir au moins `0x0200000000000000`. Le packager promeut les valeurs nulles à cette base ; une valeur explicite permet de vérifier les métadonnées sans les modifier.

## Compiler et empaqueter

Depuis la racine du dépôt QUENTIN STORE :

```powershell
dotnet build native-pkg/runner/Runner.csproj -c Release --configfile native-pkg/NuGet.Config
dotnet native-pkg/runner/bin/Release/net10.0/Runner.dll `
  build/native-ps5/PPSA99741 `
  dist/QUENTIN-STORE-PS5.pkg `
  build/native-pkg-validation
```

Remplacer le premier chemin par le dossier réellement préparé si la compilation utilise un autre répertoire. Les trois paramètres sont, dans l'ordre : dossier natif, fichier PKG final, dossier de validation. `NuGet.Config` désactive les sources de paquets : aucune bibliothèque externe NuGet n'est nécessaire.

Le programme échoue si une vérification échoue. Il vérifie le conteneur FIH version 3, son contenu PS5 de type application de base, son identité, l'extraction du système de fichiers, tous les fichiers d'entrée par SHA-256, la conservation des sources, et les modules natifs ELF64 x86-64 FreeBSD de types `0xFE10` et `0xFE18`. Il écrit un rapport `validation.json` et conserve une extraction indépendante dans le dossier de validation. Il rejette les métadonnées PS4 `param.sfo` et un module `right.sprx` non examiné.

Ces vérifications ne reproduisent pas toutes les règles cryptographiques de la console. La prise en charge des paquets PS5 et des exécutables natifs dépend du jailbreak, de ses composants et du firmware.

## Sources et modifications

La bibliothèque est dérivée de [SvenGDK/LibProsperoPKG](https://github.com/SvenGDK/LibProsperoPKG), commit `748eabf1b7d17819528cabf367d8e27109d8fce3`, sous GPL-3.0-or-later. Les licences et attributions sont conservées dans `LibProsperoPkg-LICENSE` et `LibProsperoPkg-NOTICE`. Les sources du CLI sont aussi sous GPL-3.0-or-later.

Les changements locaux sont détaillés dans `LOCAL-PATCH.md` : suppression du module binaire `right.sprx` dont la provenance n'a pas été établie, puis correction du lecteur pour reconstruire un espace logique de remplissage couvrant plusieurs blocs. Le générateur du conteneur et de l'image reste celui de la bibliothèque publique. Aucun fichier d'exemple précompilé, `bin/` ou `obj/` de l'amont n'est nécessaire dans cette distribution des sources.
