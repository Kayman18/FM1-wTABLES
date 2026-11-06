#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/obj82asan
CC="${CC:-cc}"
CFLAGS="-O1 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS="detect_leaks=0"
for src in fm1/app/pw_*.c; do
 name="${src##*/}"; name="${name%.c}"
 "$CC" $CFLAGS -c "$src" -o "native/obj82asan/$name.o"
done
objects="$(find native/obj82asan -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $CFLAGS -c tests/flash49_host.c -o native/obj82asan/flash49_host.o
"$CC" $CFLAGS -pthread -c tests/dual40_pthread.c -o native/obj82asan/dual40_pthread.o
"$CC" $CFLAGS -pthread tests/voices82.c native/obj82asan/flash49_host.o native/obj82asan/dual40_pthread.o $objects -lm -o native/voices82_asan
native/voices82_asan
