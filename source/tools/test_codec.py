import random,struct
from pathlib import Path
from codec import encode,decode,pack,unpack
rng=random.Random(48)
for n in (16,32,64,16368):
 cases=[[0]*n,[-32768]*n,[32767]*n,[(-32768 if i%2 else 32767) for i in range(n)],[(i%65536)-32768 for i in range(n)],[rng.randrange(-32768,32768) for _ in range(n)]]
 for v in cases:
  encoded=encode(v);assert decode(encoded,n)==struct.pack('<%dh'%n,*v)
  for bad in [encoded[:-1],encoded+b'\0']:
   try:decode(bad,n)
   except ValueError:pass
   else:raise AssertionError('accepted malformed stream')
raw=Path('reference.pwbank').read_bytes()[256:];data,banks=pack(raw);assert unpack(data)==banks
for i in [0,8,16,144,252,256,len(data)-1]+[rng.randrange(len(data)) for _ in range(100)]:
 bad=bytearray(data);bad[i]^=1
 try:unpack(bad)
 except ValueError:pass
 else:raise AssertionError('accepted corruption')
print('PASS: factory samples, 24 zero/extreme/ramp/random fixtures, truncation/trailing rejection, 107 corrupt-container cases.')
