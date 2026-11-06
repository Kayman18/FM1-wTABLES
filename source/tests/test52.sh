#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test52
cc -O1 -g -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_banks52.c tests/flash49_host.c fm1/app/pw_tables.c -o native/banks52
ASAN_OPTIONS=detect_leaks=0 native/banks52 > TEST52-banks.log 2>&1
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_dual52.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual52
ASAN_OPTIONS=detect_leaks=0 native/dual52 > TEST52-dual.log 2>&1
fm1/app/build/host/pwcore_host tests/ui52.txt validation/test52 > TEST52-ui.log 2>&1
OPENBLAS_NUM_THREADS=1 python3 tools/convert_virus52.py > TEST52-conversion.log 2>&1
cmp data/virus-conversion/virus-banks.pwlc data/virus-banks.pwlc
sha256sum data/virus-conversion/virus-banks.pwlc data/virus-banks.pwlc > TEST52-reproduce.log

cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include -I fm1/app tests/levels52.c tests/flash49_host.c $sources -lm -o native/levels52
native/levels52 > TEST52-levels.csv
