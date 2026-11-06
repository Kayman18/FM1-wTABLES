#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
: "${JIELI_TOOLCHAIN:?Provide matching JieLi PI32v2 compiler directory}"
: "${AC79_SDK:?Provide licensed JieLi AC79 SDK directory}"
export X0X_FONTSET="${X0X_FONTSET:-barlow}"
sh fm1/build.sh --release 0.68.0
