"""Validate TEST49 expanded lossless bank container."""
import struct,zlib
from codec import decode

def unpack(data):
 if len(data)<512 or data[:8]!=b'PWLC4901':raise ValueError('wrong format')
 if struct.unpack_from('<II',data,8)!=(1,16368):raise ValueError('wrong geometry')
 total,crc,count=struct.unpack_from('<III',data,480)
 if not 1<=count<=29 or total!=len(data) or total>0x45000:raise ValueError('size or count')
 if zlib.crc32(data[:508])!=struct.unpack_from('<I',data,508)[0] or zlib.crc32(data[512:])!=crc:raise ValueError('container checksum')
 result=[];end=512
 for i in range(count):
  off,size,decoded_crc,packed_crc=struct.unpack_from('<4I',data,16+16*i)
  if off!=end or size>len(data)-off:raise ValueError('range')
  c=data[off:off+size]
  if zlib.crc32(c)!=packed_crc:raise ValueError('packed checksum')
  raw=decode(c)
  if zlib.crc32(raw)!=decoded_crc:raise ValueError('decoded checksum')
  result.append(raw);end=off+size
 if end!=len(data):raise ValueError('trailing bytes')
 return result
