#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
cc -O2 -ffp-contract=off -shared -fPIC -o native/libchord.so fm1/app/pw_chord.c
cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwperformance.so fm1/app/pw_performance.c
python3 - <<'PY'
import runpy
for name in ['compare_chord38','compare_pitch38']:
 runpy.run_path('tests/'+name+'.py',run_name='__main__')
PY
