# Installation sur une PS5 13.60

Cette procédure vise un environnement déjà jailbreaké. QUENTIN STORE n'exploite pas la console. Le firmware 13.60 est une cible non validée pour ces ELF. Ne modifier ni firmware ni protections réseau pour tenter de résoudre un problème du store.

## PKG PS5 natif — version de test

Télécharger [QUENTIN-STORE-PS5.pkg](https://github.com/tomoff1/quentin-store/releases/download/v0.1.2/QUENTIN-STORE-PS5.pkg). Il contient un lanceur PS5 (`PPSA99741`, version `01.000.000`) et le store ELF 0.1.2 embarqué. Le lanceur transmet ce store au chargeur local sur le port 9021 ; il requiert donc ce chargeur actif, puis se ferme pour laisser la vidéo au store. Le PKG ne constitue pas encore un port autonome du store vers le contexte d'une application système.

1. Activer l'environnement jailbreak et le chargeur ELF 9021 déjà utilisés sur la console.
2. Installer **ce PKG PS5** avec l'installateur de la console/ps5upload s'il accepte ce format. Le port 9021 sert à transmettre les ELF, pas à installer un PKG.
3. Ouvrir **QUENTIN STORE PS5** depuis le menu. C'est une nouvelle application ; les anciennes tuiles **QUENTIN STORE Launcher** PS4 ne sont pas cette version.

Une notification « lanceur natif execute » indique que l'exécutable PS5 a réellement commencé. « ELF transmis » confirme seulement le transfert. Le store affiche ensuite sa propre notification « Demarrage du store 0.1.2 », puis une notification à sa première image. Si le chargeur est inaccessible ou l'ELF illisible, le lanceur affiche une page d'erreur.

L'installation et l'exécution de ce PKG natif sur 13.60 restent à tester. Les anciens PKG PS4 1.00 et 1.01 ont été **refusés par la console de l'utilisateur** avec « Ce jeu PS4 n'est pas jouable sur PS5 » ; ils ne sont plus recommandés. La compilation, le format natif et les essais PC ne prouvent pas l'acceptation par la console.

## Un seul fichier a envoyer

Telecharger uniquement `quentin-store.elf` depuis la release v0.1.2. La police, les certificats HTTPS et un catalogue initial sont integres. Aucun transfert FTP de fichiers annexes n'est necessaire.

Au premier lancement, le store cree automatiquement `/data/quentin-store/`, sa configuration, ses certificats et son catalogue. Les parametres et l'historique existants sont conserves. Internet permet d'actualiser le catalogue. Le SHA-256 est disponible dans `SHA256SUMS` sur la release.

## Lancer l'ELF

Conserver votre ELF Loader déjà compatible avec votre jailbreak. Le chargeur ps5-payload-dev/elfldr documente le port TCP 9021 ; la seule présence de ce port ne garantit pas les privilèges et modules nécessaires pour SDL sur 13.60.

Depuis le dossier de distribution décompressé sur PC (le script est inclus) :

```powershell
./scripts/send-elf.ps1 -ConsoleIP ADRESSE_IP_PS5 -Port 9021
```

Le script transmet l'ELF, puis ferme le socket. Son message de succès confirme l'envoi, pas le lancement. Si votre chargeur prend en charge `file:`, vous pouvez lui envoyer `file:/data/quentin-store/quentin-store.elf` suivi d'un saut de ligne. Utiliser le protocole exact de votre loader.

Le script détecte l'ELF à la racine de la distribution, dans `dist/` du projet source ou dans `build/ps5/` après compilation. Pour choisir un autre ELF, ajouter `-ElfPath CHEMIN_COMPLET`.

**ShadowMountPlus** : copier le seul payload ELF dans `/data/homebrew/` ne le transforme pas en application du menu. Le PKG PS5 décrit ci-dessus utilise une chaîne distincte avec un véritable exécutable natif ; aucune image exFAT n'est livrée.

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

Si l'écran reste noir : noter les notifications reçues et récupérer `/data/quentin-store/startup.log`, `/data/quentin-store/diagnostic.log` s'ils existent, ainsi que les logs du loader. `startup.log` indique la dernière étape atteinte avant l'affichage. Ne conclure à la compatibilité 13.60 qu'après ces essais. Le diagnostic local ne collecte pas vos identifiants et ne transmet rien sur le réseau.
