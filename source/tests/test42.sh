#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
bash tests/test40-thread.sh > TEST42-thread.log 2>&1
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -DDC_PTHREAD -pthread -I fm1/base/include tests/test_dual42_presets.c tests/dual40_pthread.c $sources -lm -o native/dual42-presets
native/dual42-presets > TEST42-presets.log
cc -O1 -g -ffp-contract=off -DDC_PTHREAD -pthread -fsanitize=thread -I fm1/base/include tests/test_dual42_race.c tests/dual40_pthread.c $sources -lm -o native/dual42-tsan
native/dual42-tsan > TEST42-tsan.log 2>&1
fm1/app/build/host/pwcore_host tests/presets42-ui.txt validation/test41 > TEST42-ui.log 2>&1
