#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test32
fm1/app/build/host/pwcore_host tests/arp32-screens.txt validation/test32
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test32; done
python3 tests/check_arp32_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test32
python3 tests/check_arp32_stop.py
