#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test41
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include tests/test_live43.c tests/dual40_pthread.c $sources -lm -o native/live43-thread
ASAN_OPTIONS=detect_leaks=0 native/live43-thread > TEST43-live-thread.log
fm1/app/build/host/pwcore_host tests/live43-ui.txt validation/test41 > TEST43-ui.log 2>&1
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include tests/test_dual42_presets.c tests/dual40_pthread.c $sources -lm -o native/dual43-presets
native/dual43-presets > TEST43-presets.log
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include tests/test_dual42_race.c tests/dual40_pthread.c $sources -lm -o native/dual43-tsan
native/dual43-tsan > TEST43-tsan.log 2>&1
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include tests/test_bloom.c $sources -lm -o native/bloom43
ASAN_OPTIONS=detect_leaks=0 native/bloom43 > TEST43-bloom.log
