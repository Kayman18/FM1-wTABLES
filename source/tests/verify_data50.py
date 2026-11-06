from pathlib import Path
import json,csv
r=Path(__file__).resolve().parents[1]
rows=json.loads((r/'presets/fm1-presets.json').read_text());old=json.loads((r/'presets/test49-reference.json').read_text());assert len(rows)==39
assert rows[:27]==[x for x in old if x['name']!='Simplemind FM1'];assert len(set(x['name'] for x in rows))==39
levels=list(csv.reader((r/'TEST50-levels.csv').open()));assert len(levels)==24
for row in levels:assert 28<=int(row[0])<=39 and 0<float(row[3])<.8 and 0<float(row[4])<.8 and int(row[5])==0,row
print('PASS: Simplemind removed, other 27 earlier sounds unchanged; 12 additions, all single/six-note level auditions nonzero and unclipped.')
