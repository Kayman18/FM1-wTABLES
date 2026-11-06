#!/bin/sh
set -eu
mkdir -p native validation/test53
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_engine53.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/engine53
ASAN_OPTIONS=detect_leaks=0 native/engine53 > TEST53-engine.log 2>&1
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined -I fm1/base/include -I fm1/app tests/test_dual53.c tests/flash49_host.c tests/dual40_pthread.c $sources -lm -o native/dual53
ASAN_OPTIONS=detect_leaks=0 native/dual53 > TEST53-dual.log 2>&1
