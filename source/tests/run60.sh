#!/bin/sh
set -eu
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
for item in testrik60 rests57 voices58 test_dual59; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/$item.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/$item
 ASAN_OPTIONS=detect_leaks=0 native/$item > TEST60-$item.log 2>&1
 echo "$item: PASS"
done
