#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test36
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest36engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest35ref.so tests/baseline35/test_engine.c tests/baseline35/pw_delay.c $(printf "%s\n" "$sources" | sed '/pw_delay.c/d')
python3 tests/compare_gain36_bypass.py
for test in gain_delay five_chords arp_clock; do
 cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o "validation/test36/test-$test" "tests/test_$test.c" $sources -lm
 ASAN_OPTIONS=detect_leaks=0 "validation/test36/test-$test"
done
