#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
bash tests/run70.sh
CC="${CC:-cc}"
CFLAGS="-O1 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS=detect_leaks=0
objects="$(find native/obj69asan -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $CFLAGS -pthread tests/test_release71.c native/obj69asan/flash49_host.o native/obj69asan/dual40_pthread.o $objects -lm -o native/test_release71_asan
native/test_release71_asan
