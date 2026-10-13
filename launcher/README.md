# QUENTIN STORE Launcher - experimental

This is a PS4 FPKG launcher intended for an etaHEN-enabled PS5 that supports PS4 homebrew PKGs. It bundles the PS5 QUENTIN STORE v0.1.1 ELF and sends its bytes to the existing local ELF loader on 127.0.0.1:9021. It is not a native PS5 PKG and does not implement an ELF loader or fix the store's unresolved startup issue.

Install via the existing compatible PKG installer. Keep etaHEN and its ELF loader enabled. Launch QUENTIN STORE Launcher from the menu. A notification reports transfer success or failure; transfer success does not confirm store execution. Physical PS5 13.60 installation and execution are not tested.

GPL-3.0-or-later. Launcher sources are in launcher/main.c and scripts/build_pkg_launcher.py. Store source: https://github.com/tomoff1/quentin-store/releases/tag/v0.1.1
OpenOrbis toolchain v0.5.4: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/tag/v0.5.4
LibOrbisPkg: https://github.com/maxton/LibOrbisPkg (LGPL-3.0).

## 1.01 compatibility experiment
The PS5 rejects launcher 1.00 before executing it while another PS4 PKG runs in the same jailbreak session. APP_TYPE is changed from 1 to 0, following the value in the public LightningMods PS4 Homebrew Store release 4.5 param.sfo. APP_VER and VERSION are now 01.01. The other classification fields and title ID are unchanged. This isolates a metadata hypothesis; PS5 acceptance and store startup remain unconfirmed.
