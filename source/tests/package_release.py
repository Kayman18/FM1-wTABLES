"""Create the TEST52 full M-Flasher image after completed validation."""
from pathlib import Path
import subprocess,sys,hashlib
r=Path(__file__).resolve().parents[1]
for name in ['banks','dual','ui','reproduce']:
 s=(r/f'TEST52-{name}.log').read_text()
 assert s.strip() and not any(x in s for x in ['FAIL','runtime error:','WARNING: ThreadSanitizer','ERROR: AddressSanitizer','AssertionError','Traceback']),name
import csv
levels=list(csv.reader((r/'TEST52-levels.csv').open()));assert len(levels)==96 and all(float(x[5])>0 and int(x[7])==0 for x in levels)
assert 'identity FM-1_13005200' in (r/'TEST52-build.log').read_text()
if len(sys.argv)!=3:raise SystemExit('Usage: python3 tests/package_release.py BASE_BACKUP.bin OUTPUT.bin')
subprocess.run([sys.executable,str(r/'tools/make_mflasher52.py'),*sys.argv[1:]],check=True)
