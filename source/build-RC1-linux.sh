#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  cat <<'USAGE'
Usage: ./build-RC1-linux.sh [KNOWN-COMPATIBLE-1M.bin [OUTPUT-M-Flasher.bin]]

Build wTABLES RC1 application and FWSC. With a compatible full 1 MiB backup,
also build FM1-wTABLES-RC1-M-Flasher.bin (or the supplied output path).
Paths are relative to this source directory unless absolute.
Set JIELI_TOOLCHAIN to the compatible JieLi toolchain root first.
USAGE
  exit 0
fi
if (( $# > 2 )); then
  printf '%s\n' 'Usage: ./build-RC1-linux.sh [KNOWN-COMPATIBLE-1M.bin [OUTPUT-M-Flasher.bin]]' >&2
  exit 2
fi
if (( $# > 0 )) && [[ ! -r "$1" ]]; then
  printf 'Cannot read the full-image base: %s\n' "$1" >&2
  exit 2
fi

: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the compatible JieLi toolchain root (contains pi32v2/ and common/)}"
export AC79_SDK="${AC79_SDK:-$PWD/build-deps/sdk}"
export X0X_FONT_DIRS="${X0X_FONT_DIRS:-$PWD/build-deps/fonts}"
export X0X_FONTSET=barlow

sh fm1/build.sh --release 0.83.0-rc1 --display-version 'wTABLES RC1'
if (( $# > 0 )); then
  "${PYTHON:-python3}" tools/make_mflasher66.py "$1" "${2:-FM1-wTABLES-RC1-M-Flasher.bin}"
fi
