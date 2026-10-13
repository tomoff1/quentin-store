# Validation réelle

Réalisée sur Windows 11 le 13 octobre 2026.

| Vérification | Résultat observé |
|---|---|
| CMake + CTest, logique du catalogue et sécurité | 1 suite passée, 36 assertions |
| Moteur C++ réel via serveur HTTPS local | 20 contrôles passés |
| Certificat absent et identité TLS incorrecte | Refus observés, aucune désactivation TLS |
| HTTP 404 et JSON > 1 Mio | Refus observés |
| Téléchargement complet + SHA-256 + historique | Réussi |
| Reprise à l'octet 1234 | Header Range et contenu final vérifiés |
| Serveur ignorant Range | Nouveau téléchargement complet, hash correct |
| Content-Range incohérent | Refus, préfixe partiel intact |
| Fichier corrompu, trop court ou trop long | Refus observés |
| Coupure réseau | Partiel conservé, reprise à l'octet 50000 observée, hash final correct |
| Annulation et identifiant ../escape | Refus observés |
| Compilation croisée C++ pour PS5 | Deux ELF produits par clang/lld 18.1.8, SDK public v0.44 |
| Inspection ELF | ELF64 little endian x86-64 PIE, segments alignés sur 0x4000, dépendances système SCE |
| Interface réelle sur PC | Exécution SDL, capture incluse dans site/screenshot.png |
| Scripts Python | Compilation syntaxique réussie ; relink des objets fourni testé, ELF identique |
| Scripts PowerShell | Analyse syntaxique réussie |
| Script de transfert livré dans l'archive | Archive extraite, script PowerShell exécuté contre un serveur TCP local : 18 069 120 octets identiques reçus ; aucune PS5 contactée |
| PS5 physique firmware 13.60 | ELF transmis sans affichage selon l'utilisateur ; PKG PS4 1.00/1.01 refusés avec « Ce jeu PS4 n'est pas jouable sur PS5 » |
| GitHub Actions | Tests et compilation PS5 réussis sur Ubuntu 24.04 ; [run vérifié](https://github.com/tomoff1/quentin-store/actions/runs/38352197453) |
| Catalogue public et GitHub Releases | Chargement HTTPS et téléchargement de Quentin Blocks vérifiés avec le moteur C++ réel, SHA-256 correct |
| Cloudflare Pages | [Site public déployé](https://quentin-store.pages.dev/), ouverture vérifiée dans le navigateur, bouton Télécharger actif |

Les captures sont celles du programme exécuté sur PC, pas des images générées ni une preuve d'exécution sur console. La suite réseau utilise `src/network.cpp`, également lié dans le payload PS5 ; elle ne prouve pas la compatibilité de la pile PS5 avec ces bibliothèques.

Les tests sur console qui restent indispensables sont détaillés dans INSTALLATION-PS5.md. Le protocole ELF Loader, la disponibilité des modules système, les permissions de vidéo, le système de fichiers et TLS doivent être validés sur la machine réelle.

## Version 0.1.1 autonome

ELF PS5 recompile avec police, CA et catalogue integres. Test du client SDL reel dans un dossier vierge sans fichier annexe : creation automatique, rendu avec police en memoire, CA identique et catalogue charge. Second lancement : configuration existante et historique conserves. Le test utilise SDL dummy sur PC et ne confirme pas une execution PS5. Suite core : 36 assertions reussies. Transfert TCP local du nouvel ELF : octets identiques.

## Version 0.1.2 et lanceur natif PS5

Le dossier de données PS5 ne dépend plus des arguments hérités du processus du chargeur. Des notifications et `startup.log` indiquent le démarrage, l'accès aux données, l'initialisation SDL et la première image. Compilation PS5 réussie. Le client réel sur PC conserve les ressources et paramètres ; un pilote SDL volontairement invalide provoque une sortie contrôlée de code 3 et une erreur persistée. CTest : suite passée.

Le lanceur est compilé avec le CRT d'application PS5 et le convertisseur public ps5-native-app-boilerplate, séparément du CRT payload du store. Il possède un exécutable PS5 `0xFE10`, un FSELF PS5 standard de magic `0xEEF51454` et un module `libc.prx` écrit par ce projet amont et reconstruit deux fois avec un résultat identique. L'ELF contenu dans le FSELF conserve son empreinte et ses métadonnées natives ; l'intégrité du FSELF est vérifiée par le convertisseur. Il affiche une erreur si le chargeur 9021 est absent et utilise la fermeture système de l'application après transfert. Le test PC exécute le même code de transfert contre un serveur TCP local : les **18 386 008 octets** du store arrivent identiques malgré des écritures partielles de 511 octets ; envoi retournant une erreur, chargeur absent, ELF invalide et fichier absent sont refusés. Les API PS5 y sont adaptées aux API PC pour ce test : il ne valide pas leur fonctionnement sur console.

Le constructeur produit un conteneur PS5 FIH debug version 3, de type application `0x20`. Les contrôles structurels passent. La réextraction relit tous les fichiers d'origine (eboot, runtime, payload, param.json, icône) avec les mêmes SHA-256. Un défaut du lecteur amont qui avançait de seulement 256 Kio dans une zone de remplissage plus longue a été corrigé ; cette correction touche l'extraction, sans changer les octets générés par le constructeur. Le module `right.sprx` opaque est absent du PKG et des sources modifiées livrées.

Le PKG natif et son démarrage réel sur firmware 13.60 restent non validés sur console. Aucun envoi TCP ni vérification du format ne prouve que le store s'affiche.
