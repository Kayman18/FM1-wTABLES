#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test34
fm1/app/build/host/pwcore_host tests/clock34-screens.txt validation/test34
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle up-8ve down-8ve alt-8ve alt2-8ve; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test34; done
python3 tests/check_arp34_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test34
python3 tests/check_arp34_stop.py

for port in usb trs; do fm1/app/build/host/pwcore_host "tests/clock-$port.txt" validation/test34; done
python3 tests/check_clock_audio.py
