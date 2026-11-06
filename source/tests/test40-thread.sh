#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest39ref.so tests/baseline39/test_engine.c $sources
cc -O2 -ffp-contract=off -DDC_PTHREAD -shared -fPIC -pthread -I fm1/base/include -o native/libtest40thread.so fm1/app/test_engine.c tests/dual40_pthread.c $sources
python3 tests/compare_dual40_thread.py
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include -o native/dual40-tsan tests/test_dual40_race.c tests/dual40_pthread.c $sources -lm
native/dual40-tsan
printf '%s\n' 'PASS: ThreadSanitizer concurrent eight-voice test.'
