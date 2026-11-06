#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_core.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -I fm1/app tests/baseline43/test_engine.c tests/baseline43/pw_core.c $sources -lm -o native/libtest43ref.so
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include fm1/app/test_engine.c fm1/app/pw_core.c tests/dual40_pthread.c $sources -lm -o native/libtest44thread.so
python3 tests/compare_dual44.py > TEST44-compare.log
python3 tests/compare_seq44.py > TEST44-seq-compare.log
bash tests/test43.sh > TEST44-regression.log 2>&1
