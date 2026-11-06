#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to directory containing pi32v2 and common folders}"
: "${AC79_SDK:?Set AC79_SDK to licensed SDK directory containing cpu/wl82/tools/cfg_tool.bin and cpu/wl82/tools/cfg/eq_cfg_hw.bin}"
# User-provided fonts need to be placed in fm1/base/assets/fonts, see FONT_ASSETS_NOT_INCLUDED.txt.
sh fm1/build.sh --release 0.67.0
# To build CPU1 64-frame variant, add o2_cflags = ["-DPW_VOICE_CHUNK=64u"] to fm1/app/fm1.toml and use --release 0.67.1
