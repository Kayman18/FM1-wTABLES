from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[1]
for name in ['editor','dual','ui']:
 s=(r/f'TEST56-{name}.log').read_text()
 assert ('PASS:' in s if name!='ui' else 'host:' in s),name
 assert not any(x in s for x in ['FAIL','runtime error:','ERROR: AddressSanitizer','AssertionError','Traceback','Aborted']),name
assert 'identity FM-1_13005600' in (r/'TEST56-build.log').read_text()
subprocess.run([sys.executable,str(r/'tools/make_mflasher56.py'),*sys.argv[1:]],check=True)
