#!/bin/sh
# TEST66 host regression: compile shared audio DSP once and link each test.
# ASan/UBSan remain enabled across all compiled C units.
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/obj66
CC="${CC:-cc}"
CFLAGS="-O2 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS="detect_leaks=0"
python3 tests/test_bank_labels66.py
# The corresponding pw_*.c files each have different source dependencies, but
# common object reuse makes this suite practical on a normal Linux laptop.
for src in fm1/app/pw_*.c; do
 name="${src##*/}"
 name="${name%.c}"
 "$CC" $CFLAGS -c "$src" -o "native/obj66/$name.o"
done
objects="$(find native/obj66 -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $CFLAGS tests/test_akwf16.c native/obj66/pw_core.o native/obj66/pw_tables.o native/obj66/pw_presets.o -lm -o native/test_akwf16
native/test_akwf16
"$CC" $CFLAGS -c tests/flash49_host.c -o native/obj66/flash49_host.o
"$CC" $CFLAGS -pthread -c tests/dual40_pthread.c -o native/obj66/dual40_pthread.o
for name in testrik60 test_matrix61 rests61 voices61 test_dual61; do
 "$CC" $CFLAGS -pthread "tests/$name.c" native/obj66/flash49_host.o native/obj66/dual40_pthread.o $objects -lm -o "native/$name"
 "native/$name"
done
"$CC" $CFLAGS tests/test_bloom_clock62.c native/obj66/pw_bloom.o -lm -o native/test_bloom_clock62
native/test_bloom_clock62
"$CC" $CFLAGS tests/test_overlay62.c -lm -o native/test_overlay62
native/test_overlay62
"$CC" $CFLAGS tests/cloud_stability66.c native/obj66/pw_reverb.o -lm -o native/cloud_stability66
native/cloud_stability66
"$CC" $CFLAGS -pthread tests/cloud_integration66.c native/obj66/flash49_host.o native/obj66/dual40_pthread.o $objects -lm -o native/cloud_integration66
native/cloud_integration66
echo 'PASS: TEST66 Clouds + AKWF bank/preset/overlay/voice regression suite completed'
