#!/bin/sh
set -eu
mkdir -p native validation/test54
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
for item in ratchet54 test_engine53 test_dual54; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/$item.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/$item
 ASAN_OPTIONS=detect_leaks=0 native/$item > TEST54-$item.log 2>&1
done
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include -I fm1/app tests/levels54.c tests/flash49_host.c $sources -lm -o native/levels54
native/levels54 > TEST54-levels.log 2>&1
