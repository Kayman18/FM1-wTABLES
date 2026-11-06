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

# Bit-exactness gates for the CPU work. Both render the engine and print one
# hash of every sample; the expected values are the pre-optimization output, so
# any change in arithmetic (as opposed to speed) shows up here. Built without
# the sanitizers, in their own object directory, so they stay quick.
GOLDEN_EXPECT="WTGOLDEN 9fa1149245be5551"
FUZZ_EXPECT="WTFUZZ d36cbbc5612ae044"
EXFLAGS="-O2 -ffp-contract=off -std=gnu11 -DPLAT_HOST -DDC_PTHREAD -I fm1/base/include -I fm1/app"
mkdir -p native/obj82exact
for src in fm1/app/pw_*.c; do name="${src##*/}"; name="${name%.c}"
 "$CC" $EXFLAGS -c "$src" -o "native/obj82exact/$name.o"
done
exobjects="$(find native/obj82exact -maxdepth 1 -name 'pw_*.o' -print | sort)"
"$CC" $EXFLAGS -c tests/flash49_host.c -o native/obj82exact/flash49_host.o
"$CC" $EXFLAGS -pthread -c tests/dual40_pthread.c -o native/obj82exact/dual40_pthread.o
"$CC" $EXFLAGS -pthread tests/golden82_hash.c native/obj82exact/flash49_host.o native/obj82exact/dual40_pthread.o $exobjects -lm -o native/golden82_hash
"$CC" $EXFLAGS -pthread tests/fuzz82_settings.c native/obj82exact/flash49_host.o native/obj82exact/dual40_pthread.o $exobjects -lm -o native/fuzz82_settings
got="$(native/golden82_hash)"
[ "$got" = "$GOLDEN_EXPECT" ] || { echo "FAIL: factory-preset render hash changed: $got (expected $GOLDEN_EXPECT)"; exit 1; }
echo "PASS: 29 presets x 3 voice limits x 2 unison render hash unchanged ($got)"
got="$(native/fuzz82_settings 300)"
[ "$got" = "$FUZZ_EXPECT" ] || { echo "FAIL: randomized settings fuzz hash changed: $got (expected $FUZZ_EXPECT)"; exit 1; }
echo "PASS: 300-seed randomized settings/notes/CC/bend fuzz hash unchanged ($got)"
