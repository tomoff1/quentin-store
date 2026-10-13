# Lanceur natif PS5

Ce programme est une application PS5 native avec un eboot SCE de type `0xFE10`
dans un FSELF PS5 standard de développement (`0xEEF51454`). Il transmet le store embarqué au chargeur ELF
local sur le port 9021. Le chargeur doit déjà être actif. Un transfert réussi
ne prouve pas que le store a démarré.

L'application affiche une notification à chaque étape : entrée native,
lecture du payload, validation, connexion et transmission. En cas d'échec,
elle affiche un écran de diagnostic. Après transmission, elle demande à
Shell de la fermer via `sceSystemServiceLoadExec("exit", nullptr)` afin de
laisser VideoOut au store. Elle n'utilise pas le CRT des payloads.

La compatibilité physique avec la PS5 13.60 reste non vérifiée. Le preset de dossiers
de l'amont a été validé par son auteur sur 6.02 et 12.70, ce qui ne
garantit pas 13.60 ni l'acceptation de notre PKG. Le PKG requiert le support des paquets de développement
de l'environnement de jailbreak.

## Recompilation

Les sources complètes des outils natifs et du runtime public sont fournies
dans `third_party/sources/ps5-native-app-boilerplate-b8dc4a3fe2e05e826b037bd68824751c7468a6d7.zip`.
L'archive SHA-256 est `35c10f586bc3a2e80526920f059a851007a8e4c709915a68ee4c1d0b28c47ca8`.

Préparer LLVM 18.1.8, PS5 Payload SDK 0.44 et un compilateur hôte C++20 avec
zlib. Les chemins sont explicites et ne dépendent pas du dossier de cette
conversation. Sous Linux, installer les en-têtes hôtes zlib puis exécuter :

```sh
python3 scripts/build_native_ps5.py --sdk /opt/ps5-payload-sdk --llvm /usr/lib/llvm-18/bin
```

Sous Windows avec MinGW, fournir les en-têtes zlib et sa DLL ou bibliothèque :

```powershell
python scripts/build_native_ps5.py --sdk C:/sdk/ps5-payload-sdk --llvm C:/llvm/bin --zlib-include C:/zlib/include --zlib-library C:/mingw/bin/libz.dll
```

`--payload` choisit le store ELF à embarquer ; le défaut est
`build/ps5/quentin-store.elf`. `--output` choisit le dossier de compilation.
`--upstream` peut désigner une extraction des sources épinglées.

La sortie `build/native-ps5/PPSA99741/` contient le programme, les métadonnées,
le payload et `sce_module/libc.prx`. Ce runtime est généré deux fois et
comparé aux empreintes publiques : aucune implémentation Sony n'est copiée.
Le hash signé attendu est
`e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036`.

La création du conteneur PKG est une étape distincte. Les tests de transport
exécutent le code du lanceur sur Windows contre un récepteur TCP local,
forcent des envois partiels et vérifient les octets du payload ainsi que les
cas chargeur fermé, ELF invalide et fichier absent. Ils ne simulent pas le
chargeur du système PS5.

## Sources et licences

Le lanceur est sous GPL-3.0-or-later. L'entrée native, le convertisseur,
le rendu du diagnostic et le runtime proviennent de
[ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate/tree/b8dc4a3fe2e05e826b037bd68824751c7468a6d7),
Copyright 2026 BlackBearReloaded, GPL-3.0-or-later. Leur source complète est
incluse dans l'archive épinglée. Le runtime ne constitue pas une bibliothèque
C générale : les imports de l'application se lient aux modules du système.
