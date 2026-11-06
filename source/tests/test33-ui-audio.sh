#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test33
fm1/app/build/host/pwcore_host tests/arp33-screens.txt validation/test33
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle up-8ve down-8ve alt-8ve alt2-8ve; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test33; done
python3 tests/check_arp33_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test33
python3 tests/check_arp33_stop.py
