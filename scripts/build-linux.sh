#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
command -v clang-18 >/dev/null || { echo 'Installer: sudo apt-get install clang-18 lld-18 python3 g++'; exit 1; }
python3 scripts/bootstrap.py
python3 scripts/build_ps5.py
python3 scripts/package.py
