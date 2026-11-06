#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test40
fm1/app/build/host/pwcore_host tests/dual40-screens.txt validation/test40
bash tests/test39-ui-audio.sh
