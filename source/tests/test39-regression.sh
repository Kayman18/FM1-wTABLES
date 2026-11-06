#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest39engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest38ref.so tests/baseline38/test_engine.c $sources
python3 tests/compare_bloom39_bypass.py
