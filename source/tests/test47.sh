#!/bin/sh
# Run only when a build is requested; compiles host test binaries.
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test47
python3 tests/test_bank47.py > TEST47-bank.log
cc -O1 -g -fsanitize=address,undefined -I fm1/base/include tests/test_tables47.c fm1/app/pw_tables.c -o native/tables47
native/tables47 > TEST47-loader.log 2>&1
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline46/test_engine.c tests/flash47_host.c $sources -lm -o native/libtest46ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include -I fm1/app fm1/app/test_engine.c tests/flash47_host.c tests/dual40_pthread.c $sources -lm -o native/libtest47thread.so
python3 tests/compare_tables47.py > TEST47-compare.log
for test in matrix live bloom dual; do
 cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined,float-cast-overflow -I fm1/base/include -I fm1/app tests/test_${test}46.c tests/flash47_host.c tests/dual40_pthread.c $sources -lm -o native/${test}47
 native/${test}47 > TEST47-${test}.log 2>&1
done
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -I fm1/app tests/test_matrix46.c tests/flash47_host.c tests/dual40_pthread.c $sources -lm -o native/matrix47-tsan
native/matrix47-tsan > TEST47-tsan.log 2>&1
fm1/app/build/host/pwcore_host tests/tables47-ui.txt validation/test47 > TEST47-ui.log 2>&1
{ printf 'data data/factory-mips.pwbank 0x97000\nreboot\n'; cat tests/matrix46-ui.txt; } > native/matrix47-ui.txt
fm1/app/build/host/pwcore_host native/matrix47-ui.txt validation/test47 > TEST47-matrix-ui.log 2>&1
