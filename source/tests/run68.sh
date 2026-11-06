#!/bin/sh
# TEST68 host regression: compile shared audio DSP once and link each test.
# ASan/UBSan remain enabled across all compiled C units.
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/obj68asan
CC="${CC:-cc}"
CFLAGS="-O1 -g0 -ffp-contract=off -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
export ASAN_OPTIONS="detect_leaks=0"
python3 tests/test_bank_labels66.py
# The corresponding pw_*.c files each have different source dependencies, but
# common object reuse makes this suite practical on a normal Linux laptop.
for src in fm1/app/pw_*.c; do
 name="${src##*/}"
 name="${name%.c}"
 "$CC" $CFLAGS -c "$src" -o "native/obj68asan/$name.o"
done
objects="$(find native/obj68asan -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $CFLAGS tests/test_akwf16.c native/obj68asan/pw_core.o native/obj68asan/pw_tables.o native/obj68asan/pw_presets.o -lm -o native/test_akwf16_68
native/test_akwf16_68
"$CC" $CFLAGS -c tests/flash49_host.c -o native/obj68asan/flash49_host.o
"$CC" $CFLAGS -pthread -c tests/dual40_pthread.c -o native/obj68asan/dual40_pthread.o
for name in testrik60 test_matrix61 rests61 voices61 test_dual61; do
 "$CC" $CFLAGS -pthread "tests/$name.c" native/obj68asan/flash49_host.o native/obj68asan/dual40_pthread.o $objects -lm -o "native/${name}_68"
 "native/${name}_68"
done
"$CC" $CFLAGS tests/test_bloom_clock62.c native/obj68asan/pw_bloom.o -lm -o native/test_bloom_clock62_68
native/test_bloom_clock62_68
"$CC" $CFLAGS tests/test_overlay62.c -lm -o native/test_overlay62_68
native/test_overlay62_68
"$CC" $CFLAGS tests/cloud_stability66.c native/obj68asan/pw_reverb.o -lm -o native/cloud_stability68
native/cloud_stability68
"$CC" $CFLAGS -pthread tests/cloud_integration66.c native/obj68asan/flash49_host.o native/obj68asan/dual40_pthread.o $objects -lm -o native/cloud_integration68_asan
native/cloud_integration68_asan
"$CC" $CFLAGS -pthread tests/test_opt68.c native/obj68asan/flash49_host.o native/obj68asan/dual40_pthread.o $objects -lm -o native/test_opt68_asan
native/test_opt68_asan
echo 'PASS: TEST68 optimized Clouds + AKWF bank/preset/overlay/voice regression suite completed'


"$CC" $CFLAGS tests/branch_signature68.c native/obj68asan/pw_bloom.o -o native/branch_signature68_asan
native/branch_signature68_asan > native/branch_signature68.log
sha256sum native/branch_signature68.log
"$CC" $CFLAGS -pthread tests/test_branch_stress68.c native/obj68asan/flash49_host.o native/obj68asan/dual40_pthread.o $objects -lm -o native/test_branch_stress68_asan
native/test_branch_stress68_asan
