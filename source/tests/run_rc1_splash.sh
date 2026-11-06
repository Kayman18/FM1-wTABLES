#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/rc1-splash
export X0X_FONTSET=barlow
export X0X_FONT_DIRS="${X0X_FONT_DIRS:-$PWD/build-deps/fonts}"
if ! sh fm1/build.sh --host >native/rc1-splash/host-build.log 2>&1; then
    cat native/rc1-splash/host-build.log
    exit 1
fi
"${CC:-cc}" -O1 -g -ffp-contract=off -std=c99 -DPLAT_HOST \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Ifm1/base/device -Ifm1/base/include -Ifm1/base/lib -Ifm1/app -Ifm1/app/build/gen \
    tests/startup_rc1.c fm1/app/build/host/o2/*.o -lm -o native/rc1-splash/startup_rc1
ASAN_OPTIONS=detect_leaks=0 native/rc1-splash/startup_rc1
