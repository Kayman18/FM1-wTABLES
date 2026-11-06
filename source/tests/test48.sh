#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test48
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -fsanitize=address,undefined -I fm1/base/include tests/test_tables48.c fm1/app/pw_tables.c -o native/tables48
native/tables48 > TEST48-loader.log 2>&1
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_cache48.c tests/flash48_host.c tests/dual40_pthread.c $sources -lm -o native/cache48
native/cache48 > TEST48-cache.log 2>&1
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -I fm1/app tests/test_cache48.c tests/flash48_host.c tests/dual40_pthread.c $sources -lm -o native/cache48-tsan
native/cache48-tsan > TEST48-tsan.log 2>&1
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include -I fm1/app tests/test_dual48.c tests/flash48_host.c tests/dual40_pthread.c $sources -lm -o native/dual48
native/dual48 > TEST48-dual.log
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_tables.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline47/test_engine.c tests/baseline47/pw_tables.c tests/baseline47/flash47_host.c $sources -lm -o native/libtest47ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include -I fm1/app fm1/app/test_engine.c fm1/app/pw_tables.c tests/flash48_host.c tests/dual40_pthread.c $sources -lm -o native/libtest48thread.so
python3 tests/compare_cache48.py > TEST48-compare.log
fm1/app/build/host/pwcore_host tests/tables48-ui.txt validation/test48 > TEST48-loader-ui.log 2>&1
{ printf 'data data/factory-mips.pwlc 0x97000\nreboot\n'; cat tests/matrix46-ui.txt; } > native/ui48.txt
fm1/app/build/host/pwcore_host native/ui48.txt validation/test48 > TEST48-ui.log 2>&1
