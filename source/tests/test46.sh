#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test41 validation/test22
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -c fm1/app/pw_sloth.c -o native/sloth46.o
c++ -O2 -ffp-contract=off tests/test_sloth46.cpp native/sloth46.o -lm -o native/sloth46
native/sloth46 > TEST46-sloth.log
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline45/test_engine.c $sources -lm -o native/libtest45ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include fm1/app/test_engine.c tests/dual40_pthread.c $sources -lm -o native/libtest46thread.so
python3 tests/compare_disabled46.py > TEST46-disabled-compare.log
for test in matrix live bloom dual; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined,float-cast-overflow -I fm1/base/include tests/test_${test}46.c tests/dual40_pthread.c $sources -lm -o native/${test}46
 native/${test}46 > TEST46-${test}.log 2>&1
done
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include tests/test_matrix46.c tests/dual40_pthread.c $sources -lm -o native/matrix46-tsan
native/matrix46-tsan > TEST46-tsan.log 2>&1
fm1/app/build/host/pwcore_host tests/matrix46-ui.txt validation/test41 > TEST46-ui.log 2>&1
fm1/app/build/host/pwcore_host tests/bootloader-screens.txt validation/test22 > validation/test22/bootloader-host.log 2>&1
fm1/app/build/host/pwcore_host tests/bootloader-short-cancel.txt validation/test22 > validation/test22/bootloader-short-host.log 2>&1
python3 tests/check_bootloader.py > TEST46-boot.log
