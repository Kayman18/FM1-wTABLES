#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test39
fm1/app/build/host/pwcore_host tests/bloom39-screens.txt validation/test39
for scale in 0 1 2 3 4 5 6; do fm1/app/build/host/pwcore_host tests/bloom39-scale-$scale.txt validation/test39; done
python3 tests/check_bloom39_audio.py
bash tests/test38-ui-audio.sh
