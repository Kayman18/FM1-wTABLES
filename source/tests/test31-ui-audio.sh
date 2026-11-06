#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test31
fm1/app/build/host/pwcore_host tests/arp31-screens.txt validation/test31
for mode in up down alt alt2 step-up step-down as-played; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test31; done
python3 tests/check_arp_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test31
python3 tests/check_arp_stop.py
