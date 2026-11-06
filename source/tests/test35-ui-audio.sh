#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test35
fm1/app/build/host/pwcore_host tests/five35-screens.txt validation/test35
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle up-8ve down-8ve alt-8ve alt2-8ve; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test35; done
python3 tests/check_arp35_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test35
python3 tests/check_arp35_stop.py

for port in usb trs; do fm1/app/build/host/pwcore_host "tests/clock-$port.txt" validation/test35; done
python3 tests/check_clock35_audio.py

for name in sixth-oct seventh-oct ninth; do
 for kind in direct arp; do fm1/app/build/host/pwcore_host "tests/five-$name-$kind.txt" validation/test35; done
done
fm1/app/build/host/pwcore_host tests/five-omit.txt validation/test35
python3 tests/check_five_audio.py
