# Installation sur une PS5 13.60

Cette procédure vise un environnement déjà jailbreaké. QUENTIN STORE n'exploite pas la console. Le firmware 13.60 est une cible non validée pour ces ELF. Ne modifier ni firmware ni protections réseau pour tenter de résoudre un problème du store.

## Vérifier les fichiers

Décompresser l'archive sur PC. Comparer le résultat de `Get-FileHash -Algorithm SHA256 ./quentin-store.elf` à `SHA256SUMS` téléchargé avec la release. Le fichier doit être un ELF x86-64, pas un PKG renommé.

## Préparer les données par FTP

Sur le serveur FTP existant de la PS5, créer `/data/quentin-store/` puis copier :

| Fichier | Chemin PS5 |
|---|---|
| ca-bundle.crt | /data/quentin-store/ca-bundle.crt |
| font.ttf | /data/quentin-store/font.ttf |
| config.json | /data/quentin-store/config.json |
| catalog.json | /data/quentin-store/catalog.json |
| quentin-store.elf, si lancement par chemin | /data/quentin-store/quentin-store.elf |

Ne pas copier les ports SDK sur la console : SDL, curl et OpenSSL sont liés statiquement. Les modules système restent ceux de la console. Le dossier `downloads` est créé automatiquement. Le code refuse les liens symboliques pour les fichiers gérés sur la cible POSIX.

## Lancer l'ELF

Conserver votre ELF Loader déjà compatible avec votre jailbreak. Le chargeur ps5-payload-dev/elfldr documente le port TCP 9021 ; la seule présence de ce port ne garantit pas les privilèges et modules nécessaires pour SDL sur 13.60.

Depuis le dossier de distribution décompressé sur PC (le script est inclus) :

```powershell
./scripts/send-elf.ps1 -ConsoleIP ADRESSE_IP_PS5 -Port 9021
```

Le script transmet l'ELF, puis ferme le socket. Son message de succès confirme l'envoi, pas le lancement. Si votre chargeur prend en charge `file:`, vous pouvez lui envoyer `file:/data/quentin-store/quentin-store.elf` suivi d'un saut de ligne. Utiliser le protocole exact de votre loader.

Le script détecte l'ELF à la racine de la distribution, dans `dist/` du projet source ou dans `build/ps5/` après compilation. Pour choisir un autre ELF, ajouter `-ElfPath CHEMIN_COMPLET`.

**ShadowMountPlus** : aucun titre enregistrable n'est fourni dans cette version. Copier ce payload dans `/data/homebrew/` ne le transforme pas en application du menu PS5. La production d'un titre `eboot.bin`/exFAT requiert une autre chaîne et un test spécifique ; ne pas lui appliquer les étapes d'installation Overdrive.

## Commandes

| DualSense | Action |
|---|---|
| L1 / R1 | Onglet précédent / suivant |
| Stick gauche / D-pad | Déplacer la sélection |
| X | Ouvrir, télécharger, ajouter un caractère |
| Cercle | Fermer la fiche ; annuler un transfert ; effacer au clavier |
| Carré | Catégorie suivante |
| Triangle | Actualiser ; valider le clavier |
| L2 dans Paramètres | Télécharger la mise à jour détectée |
| Options | Fermer le clavier/la fiche, puis quitter |

Dans Recherche, Carré ouvre le clavier, puis X ouvre le résultat choisi après validation. R1 ou Triangle valide la saisie. Dans Paramètres, les deux premières lignes permettent de modifier les URL. Le clavier des URL est volontairement simple et convertit en minuscules : pour les chemins sensibles à la casse, modifier `config.json` sur PC puis le transférer par FTP après fermeture de l'application.

## Télécharger et lancer un homebrew

Un fichier vérifié arrive dans `/data/quentin-store/downloads/IDENTIFIANT.elf`. Lancer ensuite ce fichier via le loader compatible ; QUENTIN STORE ne l'exécute pas automatiquement. Un téléchargement interrompu reste `.part` et reprend à la prochaine tentative si l'identité URL/hash/taille correspond et si le serveur accepte Range. En cas de hash incorrect, le fichier partiel est supprimé, l'ELF final n'est pas remplacé.

## Mise à jour

Paramètres → Vérifier les mises à jour. Les nouveautés apparaissent ; L2 télécharge la nouvelle version et vérifie son hash. Fermer le store, copier manuellement `/data/quentin-store/downloads/quentin-store.elf` vers votre emplacement de lancement via FTP, puis relancer avec le loader. Ne remplacer un ELF en cours d'exécution qu'après fermeture. Les paramètres, le catalogue et l'historique restent dans le dossier de données.

## Tests physiques à effectuer

1. Lancement et retour propre sans plantage du loader.
2. Affichage 1280 × 720, manette, stick, D-pad, changement d'onglet.
3. TLS vers GitHub, DNS, lecture et écriture dans `/data/quentin-store`.
4. Télécharger Quentin Blocks, vérifier le fichier puis le lancer séparément.
5. Annuler un téléchargement ; tenter la reprise et vérifier l'historique.
6. Vérifier une mise à jour, lire les nouveautés et effectuer le remplacement manuel.

Si l'écran reste noir : récupérer `/data/quentin-store/diagnostic.log` s'il existe, les logs du loader, sa version et celle du jailbreak. Ne conclure à la compatibilité 13.60 qu'après ces essais. Le diagnostic local enregistre la dernière erreur, pas vos identifiants ni une collecte réseau.
