#!/usr/bin/env python3
"""Merge TEST53 encrypted application and plain tables into a supplied 1 MiB image."""
from pathlib import Path
import sys,struct,hashlib,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'fm1/base/tools'))
import fm1pkg_make as f
from codec49 import unpack
START,SIZE=0x97000,0x45000
r=Path(__file__).resolve().parents[1]
def entry(data,off):
 d=bytearray(data[off:off+32]);f.enc(d,0,32)
 assert f.crc16(d[2:])==struct.unpack_from('<H',d)[0]
 return d
base=Path(sys.argv[1]).read_bytes();assert len(base)==1048576
h=entry(base,0);isd=entry(base,0x40);assert isd[16:30]==b'isd_config.ini'
o,n=struct.unpack_from('<II',isd,4);raw=base[o:o+n];assert f.crc16(raw)==struct.unpack_from('<H',isd,2)[0]
key=f.chipkey_decode(raw[:32]);assert struct.unpack_from('<I',entry(base,0x60),4)[0]==0x4000
f.SDK=str(r/'fm1/sdk');app=(r/'fm1/app/build/pwcore.bin').read_bytes()
# Main application slot limit; TEST53 adds authorized code beyond TEST52 wave budget.
assert len(app)<=581564, "Application exceeds main slot"
virus=(r/'data/virus-banks.pwlc').read_bytes();unpack(virus)
assert len(virus)<=212992 and app.count(virus)==1, 'missing or oversized embedded Virus bank'
generated=f.flash_image(app,key)
image=bytearray(base);image[0x4000:0x93000]=generated[0x4000:0x93000]
bank=(r/"data/factory-mips.pwlc").read_bytes();unpack(bank)
image[START:START+SIZE]=bank+b"\xff"*(SIZE-len(bank))
assert all(x==255 for x in image[0xdb000:0xdc000])
assert image[0xdc000:0xde000]==base[0xdc000:0xde000]
# Verify decrypted application and directory CRCs using the preserved chip key.
d=bytearray(image[0x4000:0x93000]);f.sfc(d,0,len(d),0,key)
assert f.crc16(d[2:32])==struct.unpack_from('<H',d)[0]
blk=struct.unpack_from('<I',d,8)[0];assert f.crc16(d[32:blk])==struct.unpack_from('<H',d,2)[0]
a=d[32:64];assert f.crc16(a[2:])==struct.unpack_from('<H',a)[0]
o,n=struct.unpack_from('<II',a,4);assert d[o:o+len(app)]==app;assert f.crc16(d[o:o+n])==struct.unpack_from('<H',a,2)[0]
assert image[:0x4000]==base[:0x4000] and image[0x93000:START]==base[0x93000:START] and image[START+SIZE:]==base[START+SIZE:]
out=Path(sys.argv[2]);out.write_bytes(image)
print('PASS: 1 MiB image; preserved boot/key and all non-app/non-bank regions; encrypted app roundtrip and directory/payload CRCs; exact factory bank.')
print('Calibration reservation blank:',all(x==255 for x in base[0xdc000:0xde000]))
print('SHA256',hashlib.sha256(image).hexdigest())
