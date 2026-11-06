#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to compatible JieLi PI32v2 compiler directory}"
export X0X_FONTSET="${X0X_FONTSET:-barlow}"
sh fm1/build.sh --release 0.71.0
