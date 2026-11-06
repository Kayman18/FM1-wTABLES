#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test29
fm1/app/build/host/pwcore_host tests/hold-audio.txt validation/test29
python3 tests/check_hold_audio.py
sh tests/test27-ui-audio.sh
