#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
bash tests/run71.sh
CC="${CC:-cc}"
CFLAGS="-O1 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS=detect_leaks=0
objects="$(find native/obj69asan -maxdepth 1 -name 'pw_*.o' -print | sort)"
for test in test_osc72 test_combined72; do
 "$CC" $CFLAGS -pthread "tests/$test.c" native/obj69asan/flash49_host.o native/obj69asan/dual40_pthread.o $objects -lm -o "native/${test}_asan"
 "native/${test}_asan"
done
