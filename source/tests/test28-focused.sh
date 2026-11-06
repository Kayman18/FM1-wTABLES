#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test28
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test28/test-budget tests/test_voice_budget.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test28/test-budget
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest28engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest27ref.so tests/baseline27/test_engine.c tests/baseline27/pw_performance.c $(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_performance.c' -print)
python3 tests/compare_budget_bypass.py
