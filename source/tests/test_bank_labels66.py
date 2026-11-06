#!/usr/bin/env python3
from pathlib import Path
import json,re
r=Path(__file__).resolve().parents[1]
names=['sinharm', 'cello', 'granular', 'fmsynth', 'hdrawn', 'flute', 'eorgan', 'bw_blended', 'ebass', 'fm-calm', 'karplus-mid', 'karplus-hi', 'bw_squ', 'pwm-lead']
s=(r/'fm1/app/app.c').read_text()
m=re.search(r'static const char \*banks\[PW_TABLE_BANKS\]=\{([^}]+)\}',s)
assert m, 'bank labels missing'
labels=re.findall(r'"([^"]+)"',m.group(1))
assert labels==names, (labels,names)
assert 'PW_ANALOG_BANK_LABELS' in m.group(1), 'new table labels missing'
analog_header=(r/'fm1/app/pw_analog_bank_names.h').read_text()
analog_labels=re.findall(r'"([^"]+)"',analog_header)
assert len(analog_labels)==11 and len(set(analog_labels))==11, analog_labels
assert all(label.startswith('AN ') and len(label)<=12 for label in analog_labels)
layout=(r/'fm1/app/pw_table_layout.h').read_text()
assert re.search(r'#define\s+PW_TABLE_BANKS\s+25u',layout)
j=json.loads((r/'data/akwf16-manifest.json').read_text())
assert [b['name'] for b in j['banks']]==names
assert j['geometry']['banks']==14 and j['geometry']['frames_per_bank']==16
for b in j['banks']:
 assert len(b['files'])==16 and len(set(b['files']))==16
 assert len(b['wav_sha256'])==16
 assert b['source_directory'].removeprefix('AKWF_')==b['name']
print('PASS: all 14 legacy UI labels and 224 original source files retained; 11 unique compact Analog labels appended')
