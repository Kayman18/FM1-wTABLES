#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test49
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -fsanitize=address,undefined -I fm1/base/include tests/test_tables49.c fm1/app/pw_tables.c -o native/tables49
native/tables49 > TEST49-loader.log 2>&1
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_cache49.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/cache49
native/cache49 > TEST49-cache.log 2>&1
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -I fm1/app tests/test_cache49.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/cache49-tsan
native/cache49-tsan > TEST49-tsan.log 2>&1
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include -I fm1/app tests/test_dual49.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual49
native/dual49 > TEST49-dual.log
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include -I fm1/app tests/levels49.c tests/flash49_host.c $sources -lm -o native/levels49
native/levels49 > TEST49-levels.csv
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_tables.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline48/test_engine.c tests/baseline48/pw_tables.c tests/baseline48/flash48_host.c $sources -lm -o native/libtest48ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include -I fm1/app fm1/app/test_engine.c fm1/app/pw_tables.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/libtest49thread.so
python3 tests/compare_banks49.py > TEST49-compare.log
fm1/app/build/host/pwcore_host tests/ui49.txt validation/test49 > TEST49-ui.log 2>&1
python3 tests/verify_data49.py > TEST49-data.log
