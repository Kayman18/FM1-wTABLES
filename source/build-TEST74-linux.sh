#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the compatible JieLi PI32v2 compiler directory}"
export AC79_SDK="${AC79_SDK:-$PWD/build-deps/sdk}"
export X0X_FONT_DIRS="${X0X_FONT_DIRS:-$PWD/build-deps/fonts}"
export X0X_FONTSET=barlow
sh fm1/build.sh --release 0.74.0

if [ "$#" -gt 0 ]; then
 python3 tools/make_mflasher66.py "$1" "${2:-FM1-PolyWavetable-TEST74-GP5-Wet-Mix-M-Flasher.bin}"
fi
