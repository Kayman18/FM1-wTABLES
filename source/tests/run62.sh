#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -fsanitize=address,undefined -I fm1/app -I fm1/base/include tests/test_overlay62.c -o native/test_overlay62
ASAN_OPTIONS=detect_leaks=0 native/test_overlay62 > TEST62-test_overlay62.log 2>&1
echo "PASS test_overlay62"
cc -O1 -g -fsanitize=address,undefined -I fm1/app -I fm1/base/include tests/test_bloom_clock62.c fm1/app/pw_bloom.c -o native/test_bloom_clock62
ASAN_OPTIONS=detect_leaks=0 native/test_bloom_clock62 > TEST62-test_bloom_clock62.log 2>&1
echo "PASS test_bloom_clock62"
for item in testrik60 test_matrix61 test_presets62 rests61 voices61 test_dual61; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/$item.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/$item
 ASAN_OPTIONS=detect_leaks=0 native/$item > TEST62-$item.log 2>&1
 echo "PASS $item"
done
