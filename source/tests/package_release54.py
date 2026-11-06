from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[1]
for name in ['ratchet54','test_engine53','test_dual54','levels','dry','ui']:
 s=(r/f'TEST54-{name}.log').read_text()
 assert s.strip() and not any(x in s for x in ['FAIL','runtime error:','ERROR: AddressSanitizer','AssertionError','Traceback','Aborted']),name
 assert ('PASS:' in s if name!='ui' else 'host:' in s),name
assert 'identity FM-1_13005400' in (r/'TEST54-build.log').read_text()
subprocess.run([sys.executable,str(r/'tools/make_mflasher54.py'),*sys.argv[1:]],check=True)
