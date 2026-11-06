#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test23
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest22ref.so tests/reference_test22_engine.c $sources
python3 tests/compare_velocity23_bypass.py
fm1/app/build/host/pwcore_host tests/velocity23-screens.txt validation/test23
