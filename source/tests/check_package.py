"""Check FWSC identity and decrypted UFW header, directory and payload CRCs."""
import sys,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'fm1/base/tools'))
from fm1pkg_make import enc,crc16
raw=(ROOT/'fm1/app/build/pwcore-0.24.0.fwsc').read_bytes()
identity=''.join(chr(raw[i*48+47]-i-1) for i in range(20) if raw[i*48+47]!=0x7d)
assert identity=='FM-1_13002400',identity
logical=b''.join(raw[i*48:i*48+47] for i in range(20))+raw[960:]
hdr=bytearray(logical[:64]);enc(hdr,0,64)
headcrc,listcrc,total,count=struct.unpack_from('<HHIH',hdr)
assert headcrc==crc16(hdr[2:]);assert total==len(logical)
table=logical[64:64+count*80];assert listcrc==crc16(table)
for i in range(count):
 ent=bytearray(table[i*80:(i+1)*80]);enc(ent,0,80)
 typ,index,crc,res,off,size,padded=struct.unpack_from('<HHHHIII',ent)
 assert off+size<=len(logical);assert crc==crc16(logical[off:off+size])
print(f'PASS: {identity}; header, directory and {count} payload CRCs verified; {len(raw)} byte FWSC.')
