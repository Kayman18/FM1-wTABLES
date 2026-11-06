#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native/mixed-tables
CC="${CC:-cc}"
"$CC" -O1 -g -std=gnu11 -Wall -Wextra -Wno-misleading-indentation \
 -fsanitize=address,undefined -fno-omit-frame-pointer -DPLAT_HOST \
 -I fm1/base/include -I fm1/app tests/test_mixed_tables.c \
 -o native/mixed-tables/test_mixed_tables
# LeakSanitizer cannot inspect /proc in the managed test runner; address and
# undefined-behavior checks remain enabled for every load and malformed case.
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 native/mixed-tables/test_mixed_tables
