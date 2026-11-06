#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test39 native
cc -O2 -ffp-contract=off -c fm1/app/pw_bloom.c -o native/bloom39.o
c++ -O2 -ffp-contract=off -std=c++17 tests/bloom_reference/compare.cpp tests/bloom_reference/NativeConstants.cpp native/bloom39.o -o native/compare_bloom39
native/compare_bloom39
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
for test in bloom note_range polyphony gain_delay five_chords arp_clock; do
 cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o "validation/test39/test-$test" "tests/test_$test.c" $sources -lm
 ASAN_OPTIONS=detect_leaks=0 "validation/test39/test-$test"
done
