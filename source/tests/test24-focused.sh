#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test24
python3 tests/compare_trigger.py
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test24/test-trigger tests/test_trigger.c fm1/app/test_engine.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test24/test-trigger
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest24engine.so fm1/app/test_engine.c $sources
base=tests/baseline23
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest23ref.so "$base/test_engine.c" "$base/pw_env.c" $(find fm1/app -maxdepth 1 -name 'pw_*.c' ! -name 'pw_env.c' -print)
python3 tests/compare_trigger_bypass.py
fm1/app/build/host/pwcore_host tests/trigger-screens.txt validation/test24
for mode in gated-tap triggered-tap triggered-held; do fm1/app/build/host/pwcore_host "tests/$mode.txt" validation/test24; done
python3 tests/check_trigger_audio.py
