from pathlib import Path
import numpy as np,struct,zlib,json,sys
from codec import unpack,encode,decode
r=Path(__file__).resolve().parents[1]
original=unpack((r/'data/test48-reference.pwlc').read_bytes())
names=['VARIED','STRINGS','VOICE','GLASS','GRAIN','HARM','ORGANIC','AIR']
newnames=['PRISM','REEDS','BELLS','METAL','VOWELS','PULSE','WARM','DUST','HOLLOW','SILK','CHIME','BUZZ']
banks=list(original);details=[]
for family,name in enumerate(newnames):
 frames=[]
 for frame in range(8):
  k=np.arange(1,257,dtype=float);t=frame/7
  if family==0:a=np.exp(-k/(3+18*t))*(.35+.65*np.cos(k*(.1+t*.17))**2)
  elif family==1:a=np.where(k%2==1,1,.08)/(k**(1.3-.5*t))*np.exp(-k/45)
  elif family==2:a=np.exp(-((k-(2+9*t))/1.1)**2)+.7*np.exp(-((k-(7+21*t))/1.3)**2)+.3/k**2
  elif family==3:a=(.1+np.sin(k*(1.5+t))**8)*np.exp(-k/(6+35*t))
  elif family==4:a=(.1+np.exp(-((k-(3+9*t))/2)**2)+.6*np.exp(-((k-(13+12*t))/3)**2))/k**.8
  elif family==5:a=np.sin(np.pi*k*(.12+.34*t))/k*np.exp(-k/65)
  elif family==6:a=np.exp(-k/(2+7*t))/k**.6
  elif family==7:a=np.sin(k*k*.73+frame*.3)*np.exp(-k/(7+24*t))/k**.6
  elif family==8:a=np.where(k%3==1,1,.05)/k**(1.7-.6*t)
  elif family==9:a=np.exp(-k/(3+7*t))*(1+.4*np.sin(k*.7+t))
  elif family==10:a=np.exp(-((k-(3+17*t))/.7)**2)+.4*np.exp(-((k-(11+19*t))/.9)**2)+.15/k**2
  else:a=(.2+np.cos(k*.53)**4)*np.exp(-k/(8+50*t))/k**.5
  phase=np.zeros_like(k) if family in (1,5,6,8,9) else .18*np.sin(k*.31)*t
  spec=np.zeros(513,dtype=complex);spec[1:257]=a*np.exp(1j*phase);spec[1]+=0.2
  levels=[]
  for shift in range(1,11):
   n=2048>>shift;s=np.zeros(n//2+1,dtype=complex);s[:n//2]=spec[:n//2]*n
   if n==2:s[1]=2*spec[1].real*n
   y=np.fft.irfft(s,n=n);y-=y.mean();levels.append(y)
  scale=28000/max(np.max(np.abs(y)) for y in levels)
  frames.append([np.rint(y*scale).astype('<i2') for y in levels])
 raw=b''.join(frames[f][m].tobytes() for m in range(10) for f in range(8));compressed=encode(struct.unpack('<16368h',raw));assert decode(compressed)==raw
 proposed=512+sum(len(encode(struct.unpack('<16368h',b))) for b in banks)+len(compressed)
 print(name,len(compressed),'total',proposed)
 if proposed>0x45000:break
 banks.append(raw);names.append(name);details.append(dict(name=name,bytes=len(compressed),recipe=family,origin='Original analytic harmonic synthesis; deterministic formula in tools/expand49.py'))
header=bytearray(512);header[:8]=b'PWLC4901';struct.pack_into('<II',header,8,1,16368);payload=bytearray()
for i,b in enumerate(banks):
 c=encode(struct.unpack('<16368h',b));struct.pack_into('<4I',header,16+16*i,512+len(payload),len(c),zlib.crc32(b),zlib.crc32(c));payload+=c
struct.pack_into('<III',header,480,512+len(payload),zlib.crc32(payload),len(banks));struct.pack_into('<I',header,508,zlib.crc32(header[:508]));data=bytes(header+payload)
(r/'data/factory-mips.pwlc').write_bytes(data)
(r/'data/reference49.raw').write_bytes(b''.join(banks))
(r/'data/banks49.json').write_text(json.dumps(dict(names=names,new_banks=details,bytes=len(data),sector_bytes=(len(data)+4095)//4096*4096,limit=0x45000),indent=2))
(r/'fm1/app/pw_table_layout.h').write_text('#pragma once\n#define PW_TABLE_FLASH_START 0x97000u\n#define PW_TABLE_BYTES %du\n#define PW_TABLE_FILE_CRC 0x%08xu\n#define PW_TABLE_BANKS %du\n'%(len(data),zlib.crc32(data),len(banks)))
print('BANKS',len(banks),'BYTES',len(data),'FREE BEFORE CAL',0x45000-len(data))
