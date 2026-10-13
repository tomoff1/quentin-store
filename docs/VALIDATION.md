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
| Scripts Python | Compilation syntaxique réussie |
| Scripts PowerShell | Analyse syntaxique réussie |
| PS5 physique firmware 13.60 | **Non testée** |
| GitHub Actions | En attente du premier run public |
| Cloudflare Pages | En attente de connexion du compte |

Les captures sont celles du programme exécuté sur PC, pas des images générées ni une preuve d'exécution sur console. La suite réseau utilise `src/network.cpp`, également lié dans le payload PS5 ; elle ne prouve pas la compatibilité de la pile PS5 avec ces bibliothèques.

Les tests sur console qui restent indispensables sont détaillés dans INSTALLATION-PS5.md. Le protocole ELF Loader, la disponibilité des modules système, les permissions de vidéo, le système de fichiers et TLS doivent être validés sur la machine réelle.
