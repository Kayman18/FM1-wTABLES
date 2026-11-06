#!/usr/bin/env python3
import importlib.util, struct, tempfile, subprocess, sys, zlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('bank',root/'tools/wavetable_bank.py');bank=importlib.util.module_from_spec(spec);spec.loader.exec_module(bank)
b=bank.make_bank();assert b==(root/'data/factory-mips.pwbank').read_bytes();bank.validate(b,True)
assert bank.START+bank.SIZE==0xd7000<0xdc000
assert len(b[256:])==261888
# Every offset ends precisely at the next mip, with the final sample at region end.
for i,(offset,length) in enumerate(zip(bank.OFFSETS,bank.LENGTHS)):
 assert offset+64*length*2==(bank.OFFSETS[i+1] if i<9 else bank.SIZE)
for index in [0,8,12,16,20,24,28,32,36,40,79,80,119,120,251,252,255,256,len(b)-1]:
 damaged=bytearray(b);damaged[index]^=1
 try:bank.validate(damaged)
 except ValueError:pass
 else:raise AssertionError(('accepted corruption',index))
for short in [b'',b[:255],b[:-1],b+b'\0']:
 try:bank.validate(short)
 except ValueError:pass
 else:raise AssertionError('accepted wrong size')
# Nonuniform sentinels outside the region catch unintended broad erase/copy.
backup=bytearray((i*37+i//4096)%256 for i in range(0x100000));backup[bank.START:bank.START+bank.SIZE]=b'\xff'*bank.SIZE
patched=bank.patch_backup(bytes(backup),b)
assert patched[:bank.START]==backup[:bank.START] and patched[0xd7000:]==backup[0xd7000:]
assert patched[0xdc000:0xde000]==backup[0xdc000:0xde000]
assert bank.patch_backup(patched,b)==patched
unknown=bytearray(backup);unknown[0xc0000:0xc8000]=b'\x5a'*32768
try:bank.patch_backup(bytes(unknown),b)
except ValueError:pass
else:raise AssertionError('overwrote unrecognized data')
assert bank.patch_backup(bytes(unknown),b,True)==patched
with tempfile.TemporaryDirectory() as tmp:
 tmp=Path(tmp);a=tmp/'backup.bin';o=tmp/'out.bin';a.write_bytes(backup)
 cmd=[sys.executable,str(root/'tools/wavetable_bank.py'),'patch-backup',str(a),str(o)]
 subprocess.run(cmd,check=True,capture_output=True);assert o.read_bytes()==patched and a.read_bytes()==backup
 r=subprocess.run(cmd,capture_output=True);assert r.returncode!=0 and o.read_bytes()==patched
print('PASS: exact payload, mip boundaries, corrupt/truncated bank rejection, region-only patching, calibration preservation, unknown-data refusal, idempotent bank and no output/input overwrite.')
