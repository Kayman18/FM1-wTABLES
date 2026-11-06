#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test27
sh tests/test26-ui-audio.sh
fm1/app/build/host/pwcore_host tests/pedal-screens.txt validation/test27
for mode in legacy synth piano; do fm1/app/build/host/pwcore_host "tests/pedal-$mode.txt" validation/test27; done
python3 tests/check_pedal_audio.py
