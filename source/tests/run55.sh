#!/bin/sh
set -eu
mkdir -p native validation/test55
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -c fm1/app/pw_bloom.c -o native/bloom55.o
c++ -O2 -ffp-contract=off -std=c++17 tests/branches55.cpp tests/bloom_reference/NativeConstants.cpp native/bloom55.o -o native/branches55
native/branches55 > TEST55-branches.log 2>&1
for item in editor55 test_dual55; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/$item.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/$item
 ASAN_OPTIONS=detect_leaks=0 native/$item > TEST55-$item.log 2>&1
done
