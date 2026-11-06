#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -shared -fPIC -I fm1/base/include -o native/libtest40engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest39ref.so tests/baseline39/test_engine.c $sources
python3 tests/compare_dual40.py
