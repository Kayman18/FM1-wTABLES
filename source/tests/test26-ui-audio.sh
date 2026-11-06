#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test26
sh tests/test25-regression.sh
fm1/app/build/host/pwcore_host tests/break-screens.txt validation/test26
fm1/app/build/host/pwcore_host tests/break-audio.txt validation/test26
python3 tests/check_break_audio.py
