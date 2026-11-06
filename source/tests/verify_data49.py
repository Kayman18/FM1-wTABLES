from pathlib import Path
import sys,json,hashlib,csv
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/'tools'))
from codec49 import unpack
from codec import unpack as old
new=unpack((r/'data/factory-mips.pwlc').read_bytes());previous=old((r/'data/test48-reference.pwlc').read_bytes());assert new[:8]==previous
assert b''.join(new)==(r/'data/reference49.raw').read_bytes();assert len(set(hashlib.sha256(x).digest() for x in new))==16
rows=json.loads((r/'presets/fm1-presets.json').read_text());assert len(rows)==28;assert rows[:14]==json.loads((r/"tests/baseline48/presets.json").read_text())
levels=list(csv.reader((r/'TEST49-levels.csv').open()));assert len(levels)==28
for row in levels:assert 0<float(row[3])<.8 and 0<float(row[4])<.8 and int(row[5])==0,row
print('PASS: all 16 bank CRCs/reference bytes, original eight banks preserved, 28 presets; 14 new preset single/six-note levels nonzero and unclipped.')
