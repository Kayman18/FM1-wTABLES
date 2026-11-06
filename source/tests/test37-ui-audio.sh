#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test37
fm1/app/build/host/pwcore_host tests/five35-screens.txt validation/test37
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle up-8ve down-8ve alt-8ve alt2-8ve; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test37; done
python3 tests/check_arp37_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test37
python3 tests/check_arp37_stop.py

for port in usb trs; do fm1/app/build/host/pwcore_host "tests/clock-$port.txt" validation/test37; done
python3 tests/check_clock37_audio.py

for name in sixth-oct seventh-oct ninth; do
 for kind in direct arp; do fm1/app/build/host/pwcore_host "tests/five-$name-$kind.txt" validation/test37; done
done
fm1/app/build/host/pwcore_host tests/five-omit.txt validation/test37
python3 tests/check_five37_audio.py

fm1/app/build/host/pwcore_host tests/gain36-screens.txt validation/test37
for db in 0 -6 -40 24; do fm1/app/build/host/pwcore_host "tests/gain36-$db.txt" validation/test37; done
for mode in dry stereo pingpong; do fm1/app/build/host/pwcore_host "tests/return36-$mode.txt" validation/test37; done
python3 tests/check_gain37_audio.py

fm1/app/build/host/pwcore_host tests/poly37-screens.txt validation/test37
for n in 1 2 3 4 5 6 7 8; do fm1/app/build/host/pwcore_host "tests/poly37-unison-$n.txt" validation/test37; done
for name in sixth seventh ninth eight; do fm1/app/build/host/pwcore_host "tests/poly37-$name.txt" validation/test37; done
python3 tests/check_poly37_audio.py
