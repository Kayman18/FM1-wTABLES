#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test41
python3 tests/decode_presets41.py > TEST41-decode.log
python3 tests/generate_presets41.py
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include tests/test_presets41.c $sources -lm -o native/test-presets41
ASAN_OPTIONS=detect_leaks=0 native/test-presets41 > TEST41-presets.log
cc -O2 -ffp-contract=off -DPW_TEST_SYNC -I fm1/base/include tests/render_presets41.c $sources -lm -o native/render-presets41
native/render-presets41 validation/test41 > TEST41-audition.log
fm1/app/build/host/pwcore_host tests/presets41-ui.txt validation/test41 > TEST41-ui-audio.log 2>&1
for n in $(seq -w 1 16); do
 fm1/app/build/host/pwcore_host tests/preset41-$n.txt validation/test41 >> TEST41-ui-audio.log 2>&1
done
python3 tests/check_audio41.py > TEST41-audio-metrics.log
