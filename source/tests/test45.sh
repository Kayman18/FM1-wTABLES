#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test41 validation/test22
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_chorus.c' ! -name 'pw_delay.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline44/test_engine.c tests/baseline44/pw_chorus.c tests/baseline44/pw_delay.c $sources -lm -o native/libtest44ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include fm1/app/test_engine.c fm1/app/pw_chorus.c fm1/app/pw_delay.c tests/dual40_pthread.c $sources -lm -o native/libtest45thread.so
python3 tests/compare_dual45.py > TEST45-compare.log
python3 tests/compare_seq45.py > TEST45-seq-compare.log
python3 tests/compare_shared45.py > TEST45-shared-compare.log
bash tests/test43.sh > TEST45-regression.log 2>&1
fm1/app/build/host/pwcore_host tests/navigation45.txt validation/test41 > TEST45-navigation.log 2>&1
fm1/app/build/host/pwcore_host tests/bootloader-screens.txt validation/test22 > validation/test22/bootloader-host.log 2>&1
fm1/app/build/host/pwcore_host tests/bootloader-short-cancel.txt validation/test22 > validation/test22/bootloader-short-host.log 2>&1
python3 tests/check_bootloader.py > TEST45-boot.log
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include tests/test_shared45_race.c tests/dual40_pthread.c $sources -lm -o native/shared45-tsan
native/shared45-tsan > TEST45-tsan.log 2>&1
