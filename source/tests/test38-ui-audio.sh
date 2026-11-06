#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test38
fm1/app/build/host/pwcore_host tests/five35-screens.txt validation/test38
for mode in up down alt alt2 step-up step-down as-played random no-repeat shuffle up-8ve down-8ve alt-8ve alt2-8ve; do fm1/app/build/host/pwcore_host "tests/arp-$mode.txt" validation/test38; done
python3 tests/check_arp38_audio.py
fm1/app/build/host/pwcore_host tests/arp-stop.txt validation/test38
python3 tests/check_arp38_stop.py

for port in usb trs; do fm1/app/build/host/pwcore_host "tests/clock-$port.txt" validation/test38; done
python3 tests/check_clock38_audio.py

for name in sixth-oct seventh-oct ninth; do
 for kind in direct arp; do fm1/app/build/host/pwcore_host "tests/five-$name-$kind.txt" validation/test38; done
done
fm1/app/build/host/pwcore_host tests/five-omit.txt validation/test38
python3 tests/check_five38_audio.py

fm1/app/build/host/pwcore_host tests/gain36-screens.txt validation/test38
for db in 0 -6 -40 24; do fm1/app/build/host/pwcore_host "tests/gain36-$db.txt" validation/test38; done
for mode in dry stereo pingpong; do fm1/app/build/host/pwcore_host "tests/return36-$mode.txt" validation/test38; done
python3 tests/check_gain38_audio.py

fm1/app/build/host/pwcore_host tests/poly37-screens.txt validation/test38
for n in 1 2 3 4 5 6 7 8; do fm1/app/build/host/pwcore_host "tests/poly37-unison-$n.txt" validation/test38; done
for name in sixth seventh ninth eight; do fm1/app/build/host/pwcore_host "tests/poly37-$name.txt" validation/test38; done
python3 tests/check_poly38_audio.py

fm1/app/build/host/pwcore_host tests/range38-screens.txt validation/test38
for n in 0 12 23 24 83 84 108 120 127; do fm1/app/build/host/pwcore_host "tests/range38-note-$n.txt" validation/test38; done
for name in low-chord high-chord arp; do fm1/app/build/host/pwcore_host "tests/range38-$name.txt" validation/test38; done
python3 tests/check_range38_audio.py
