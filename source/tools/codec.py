"""Experimental lossless FM1 bank codec. Not a flashable firmware format."""
import struct,zlib
from pathlib import Path
BLOCK=32
COUNT=16368
MAGIC=b'PWLC4801'
def encode(values):
 out=bytearray()
 for start in range(0,len(values),BLOCK):
  v=list(values[start:start+BLOCK]);choices=[bytes([0])+struct.pack('<%dh'%len(v),*v)]
  for order in (1,2):
   r=v
   for _ in range(order):r=[b-a for a,b in zip(r,r[1:])]
   zz=[2*x if x>=0 else -2*x-1 for x in r];width=max(zz,default=0).bit_length()
   bits=sum(x<<(i*width) for i,x in enumerate(zz))
   choices.append(bytes([order|(width<<2)])+struct.pack('<%dh'%order,*v[:order])+bits.to_bytes((len(r)*width+7)//8,'little'))
  out+=min(choices,key=len)
 return bytes(out)
def decode(data,count=COUNT):
 out=[];pos=0
 while len(out)<count:
  n=min(BLOCK,count-len(out))
  if pos>=len(data):raise ValueError('truncated block')
  h=data[pos];pos+=1;mode=h&3;width=h>>2
  if mode==3 or width>18 or (mode==0 and width):raise ValueError('bad block header')
  size=2*n if mode==0 else 2*mode+((n-mode)*width+7)//8
  if pos+size>len(data):raise ValueError('truncated samples')
  if mode==0:v=list(struct.unpack_from('<%dh'%n,data,pos))
  else:
   v=list(struct.unpack_from('<%dh'%mode,data,pos));bits=int.from_bytes(data[pos+2*mode:pos+size],'little')
   for i in range(n-mode):
    zz=(bits>>(i*width))&((1<<width)-1);res=(zz>>1)^-(zz&1)
    value=res+v[-1] if mode==1 else res+2*v[-1]-v[-2]
    if not -32768<=value<=32767:raise ValueError('sample overflow')
    v.append(value)
  out+=v;pos+=size
 if pos!=len(data):raise ValueError('trailing data')
 return struct.pack('<%dh'%count,*out)
def pack(raw):
 # Original payload is mip-major; store independently decodable complete banks.
 banks=[bytearray() for _ in range(8)];pos=0
 for level in range(1,11):
  size=8*(2048>>level)*2
  for b in banks:b+=raw[pos:pos+size];pos+=size
 assert pos==len(raw)==261888
 header=bytearray(256);header[:8]=MAGIC;struct.pack_into('<II',header,8,1,COUNT)
 payload=bytearray()
 for i,b in enumerate(banks):
  compressed=encode(struct.unpack('<%dh'%COUNT,b))
  assert decode(compressed)==b
  struct.pack_into('<4I',header,16+i*16,256+len(payload),len(compressed),zlib.crc32(b),zlib.crc32(compressed));payload+=compressed
 struct.pack_into('<II',header,144,256+len(payload),zlib.crc32(payload));struct.pack_into('<I',header,252,zlib.crc32(header[:252]))
 return bytes(header+payload),banks
def unpack(data):
 if len(data)<256 or data[:8]!=MAGIC:raise ValueError('invalid file')
 if struct.unpack_from('<II',data,8)!=(1,COUNT):raise ValueError('version or size')
 if struct.unpack_from('<II',data,144)!=(len(data),zlib.crc32(data[256:])) or struct.unpack_from('<I',data,252)[0]!=zlib.crc32(data[:252]):raise ValueError('checksum')
 result=[];end=256
 for i in range(8):
  off,size,crc,packed_crc=struct.unpack_from('<4I',data,16+i*16)
  if off!=end or size>len(data)-off:raise ValueError('range')
  chunk=data[off:off+size]
  if zlib.crc32(chunk)!=packed_crc:raise ValueError('bank checksum')
  raw=decode(chunk)
  if zlib.crc32(raw)!=crc:raise ValueError('decoded checksum')
  result.append(raw);end=off+size
 if end!=len(data):raise ValueError('trailing data')
 return result
if __name__=='__main__':
 raw=Path('reference.pwbank').read_bytes()[256:];packed,banks=pack(raw);assert unpack(packed)==banks
 Path('factory-mips.lossless-experiment').write_bytes(packed)
 print(f'Original with header: 262144 bytes; compressed with header: {len(packed)} bytes; saving: {262144-len(packed)} bytes ({100*(1-len(packed)/262144):.2f}%)')
 print('Decoded bank: 32736 bytes. All 8 banks, 130944 samples: exact roundtrip.')
