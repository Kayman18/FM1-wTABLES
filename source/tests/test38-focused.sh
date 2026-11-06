#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test38
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest38engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest37ref.so tests/baseline37/test_engine.c tests/baseline37/pw_chord.c $(printf "%s\n" "$sources" | sed '/pw_chord.c/d')
python3 tests/compare_range38_bypass.py
for test in note_range polyphony gain_delay five_chords arp_clock; do
 cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o "validation/test38/test-$test" "tests/test_$test.c" $sources -lm
 ASAN_OPTIONS=detect_leaks=0 "validation/test38/test-$test"
done
