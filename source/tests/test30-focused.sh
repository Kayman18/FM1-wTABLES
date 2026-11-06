#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test30
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test30/test-arp tests/test_arp.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test30/test-arp
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest30engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest29ref.so tests/baseline29/test_engine.c $sources
python3 tests/compare_arp_bypass.py
