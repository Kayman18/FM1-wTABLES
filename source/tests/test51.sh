#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test51 validation/test22
export ASAN_OPTIONS=detect_leaks=0
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_reverb51.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/reverb51
native/reverb51 > TEST51-reverb.log 2>&1
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include fm1/app/pw_reverb.c -o native/libreverb51.so
python3 tests/compare_reverb51.py > TEST51-source-compare.log 2>&1
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include -I fm1/app tests/test_dual51.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual51
native/dual51 > TEST51-dual.log 2>&1
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -I fm1/app tests/test_dual51.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual51-tsan
native/dual51-tsan > TEST51-tsan.log 2>&1
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include -I fm1/app tests/legacy51.c tests/flash49_host.c $sources -lm -o native/legacy51
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -DREFERENCE50 -I fm1/base/include -I fm1/app tests/legacy51.c tests/flash49_host.c $sources -lm -o native/legacy50
native/legacy50 > native/legacy50.pcm
native/legacy51 > native/legacy51.pcm
cmp native/legacy50.pcm native/legacy51.pcm
sha256sum native/legacy50.pcm native/legacy51.pcm > TEST51-legacy.log
fm1/app/build/host/pwcore_host tests/ui51.txt validation/test51 > TEST51-ui.log 2>&1
fm1/app/build/host/pwcore_host tests/ui50.txt validation/test51 > TEST51-matrix-ui.log 2>&1
{ printf 'data data/factory-mips.pwlc 0x97000\nreboot\n'; cat tests/bootloader-screens.txt; } > native/boot51.txt
fm1/app/build/host/pwcore_host native/boot51.txt validation/test22 > TEST51-boot-host.log 2>&1
{ printf 'data data/factory-mips.pwlc 0x97000\nreboot\n'; cat tests/bootloader-short-cancel.txt; } > native/boot51short.txt
fm1/app/build/host/pwcore_host native/boot51short.txt validation/test22 > TEST51-boot-short-host.log 2>&1
python3 tests/check_bootloader.py > TEST51-boot.log
python3 tests/verify_data50.py > TEST51-data.log
