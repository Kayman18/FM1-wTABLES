#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test50 validation/test22
export ASAN_OPTIONS=detect_leaks=0
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_matrix50.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/matrix50
native/matrix50 > TEST50-matrix.log 2>&1
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -I fm1/app tests/test_matrix50.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/matrix50-tsan
native/matrix50-tsan > TEST50-tsan.log 2>&1
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include -I fm1/app tests/test_dual50.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual50
native/dual50 > TEST50-dual.log
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include -I fm1/app tests/levels50.c tests/flash49_host.c $sources -lm -o native/levels50
native/levels50 > TEST50-levels.csv
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_matrix.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline49/test_engine.c tests/baseline49/pw_matrix.c tests/flash49_host.c $sources -lm -o native/libtest49ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include -I fm1/app fm1/app/test_engine.c fm1/app/pw_matrix.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/libtest50thread.so
python3 tests/compare_matrix50.py > TEST50-compare.log
fm1/app/build/host/pwcore_host tests/ui50.txt validation/test50 > TEST50-ui.log 2>&1
{ printf 'data data/factory-mips.pwlc 0x97000\nreboot\n'; cat tests/bootloader-screens.txt; } > native/boot50.txt
fm1/app/build/host/pwcore_host native/boot50.txt validation/test22 > validation/test22/bootloader-host.log 2>&1
{ printf 'data data/factory-mips.pwlc 0x97000\nreboot\n'; cat tests/bootloader-short-cancel.txt; } > native/boot50short.txt
fm1/app/build/host/pwcore_host native/boot50short.txt validation/test22 > validation/test22/bootloader-short-host.log 2>&1
python3 tests/check_bootloader.py > TEST50-boot.log
python3 tests/verify_data50.py > TEST50-data.log
