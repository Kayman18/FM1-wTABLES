#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test30
fm1/app/build/host/pwcore_host tests/arp-screens.txt validation/test30
for mode in up down alt alt2; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test30; done
python3 tests/check_arp_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test30
python3 tests/check_arp_stop.py
sh tests/test29-ui-audio.sh
