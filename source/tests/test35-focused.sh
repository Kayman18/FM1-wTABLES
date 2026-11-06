#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test35
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test35/test-five-chords tests/test_five_chords.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test35/test-five-chords
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest35engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest34ref.so tests/baseline34/test_engine.c tests/baseline34/pw_arp.c tests/baseline34/pw_chord.c $(printf "%s\n" "$sources" | sed "/pw_arp.c/d;/pw_chord.c/d")
python3 tests/compare_chord35_bypass.py
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test35/test-clock tests/test_arp_clock.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test35/test-clock
