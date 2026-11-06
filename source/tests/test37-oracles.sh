#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native
cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwunison.so fm1/app/pw_unison.c
cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwlfophase.so fm1/app/pw_lfo_phase.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwstereochorus.so fm1/app/pw_chorus.c fm1/app/pw_stereo.c
cc -O2 -ffp-contract=off -shared -fPIC -o native/libchord.so fm1/app/pw_chord.c
cc -O2 -ffp-contract=off -shared -fPIC -o native/libbreak.so fm1/app/pw_break.c
python3 - <<'PY'
import runpy
from inspect_firmware import words
for index in (61,71):
 row=words(0x9d0889e4+16*index,4)
 assert row[0]>>16==1 and row[1]&65535==8,(index,row)
print('PASS: original Unison and Max voices descriptors both have range 1..8.')
for name in ['compare_unison','compare_lfo_phase','compare_stereo_chorus','compare_chord','compare_break37']:
 runpy.run_path('tests/'+name+'.py',run_name='__main__')
PY
