#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test25 validation/test22
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
for name in polyphony unison; do
 cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o "validation/test25/test-$name" "tests/test_$name.c" fm1/app/test_engine.c $sources -lm
 ASAN_OPTIONS=detect_leaks=0 "validation/test25/test-$name"
done
cc -O2 -fsanitize=undefined,address -o validation/test25/test-overlay tests/test_overlay.c
ASAN_OPTIONS=detect_leaks=0 validation/test25/test-overlay
fm1/app/build/host/pwcore_host tests/overlay-screens.txt validation/test22
python3 tests/check_overlays.py
fm1/app/build/host/pwcore_host tests/overlay-audio-active.txt validation/test22
fm1/app/build/host/pwcore_host tests/overlay-audio-idle.txt validation/test22
python3 tests/check_overlay_audio.py
fm1/app/build/host/pwcore_host tests/bootloader-screens.txt validation/test22 > validation/test22/bootloader-host.log
fm1/app/build/host/pwcore_host tests/bootloader-short-cancel.txt validation/test22 > validation/test22/bootloader-short-host.log
python3 tests/check_bootloader.py
fm1/app/build/host/pwcore_host tests/chord-screens.txt validation/test25
fm1/app/build/host/pwcore_host tests/chord-audio.txt validation/test25
python3 tests/check_generated_chord_audio.py
