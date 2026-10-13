QUENTIN STORE 0.1.2 — lanceur PKG PS5 natif expérimental.

Télécharger `QUENTIN-STORE-PS5.pkg`, l'installer avec un installateur acceptant les PKG PS5, activer le chargeur ELF 9021 puis ouvrir **QUENTIN STORE PS5**. Nouvelle application `PPSA99741`, version PS5 `01.000.000` : les anciennes tuiles PS4 Launcher ne sont pas cette version.

Le lanceur possède un exécutable natif PS5 et contient le store ELF 0.1.2. Il transmet l'ELF au chargeur local puis se ferme ; le chargeur 9021 reste nécessaire. Des notifications séparent le démarrage du lanceur, le transfert et le démarrage du store. Le fichier `/data/quentin-store/startup.log` enregistre les étapes précédant l'affichage.

Compilation PS5 et tests PC du code de transfert réussis, y compris écritures partielles et erreurs. Installation et affichage sur PS5 13.60 à confirmer : aucun test PC ne prouve la compatibilité physique. Les anciens PKG PS4 1.00 et 1.01 ont été refusés sur la console de l'utilisateur et ne sont plus recommandés.

Sources, scripts de reconstruction, notices et SHA-256 accompagnent les exécutables. Le PKG ne contient pas le module `right.sprx` opaque fourni par le constructeur amont.
