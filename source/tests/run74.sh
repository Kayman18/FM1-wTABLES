#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Preserve all prior regression gates, including Clouds golden PCM and workers.
bash tests/run73.sh
CC="${CC:-cc}"
CFLAGS="-O1 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS=detect_leaks=0
objects="$(find native/obj69asan -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $CFLAGS -pthread tests/test_gp5_mix74.c native/obj69asan/flash49_host.o native/obj69asan/dual40_pthread.o $objects -lm -o native/test_gp5_mix74_asan
native/test_gp5_mix74_asan
echo 'PASS: TEST74 full inherited regression and GP5 dry/wet mixer suite completed'
