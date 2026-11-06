from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[1]
for name in ['engine','dual','ui','original-branches','original-drone','trigger-regression']:
 s=(r/f'TEST53-{name}.log').read_text()
 assert s.strip() and not any(x in s for x in ['FAIL','runtime error:','ERROR: AddressSanitizer','AssertionError','Traceback','Aborted']),name
assert 'identity FM-1_13005300' in (r/'TEST53-build.log').read_text()
subprocess.run([sys.executable,str(r/'tools/make_mflasher53.py'),*sys.argv[1:]],check=True)
