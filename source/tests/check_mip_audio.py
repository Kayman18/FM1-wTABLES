import wave,numpy as np
from pathlib import Path
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(name):
 with wave.open(str(r/f'mip-{name}.wav')) as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert abs(x.astype(int)).max()<32767;assert not np.any(x[-1000:])
 y=x[int(.3*rate):int(1.3*rate),0].astype(float);p=abs(np.fft.rfft(y*np.hanning(len(y))))**2;f=np.fft.rfftfreq(len(y),1/rate);return p,f
f0=96200119*44100/4294967296
def alias(name):
 p,f=read(name);mask=f>20
 for k in range(1,int(22050/f0)+1):mask&=abs(f-k*f0)>4
 return p[mask].sum()/p[(f>20)&~mask].sum()
a,l=alias('auto-83'),alias('legacy-83');reduction=10*np.log10(l/a);assert reduction>15,(a,l,reduction)
f0=3185015*44100/4294967296
powers=[]
for name in ['auto-24','legacy-24']:
 p,f=read(name);mask=np.zeros_like(f,dtype=bool)
 for k in range(190,301):mask|=abs(f-k*f0)<2
 fundamental=p[abs(f-f0)<3].sum();powers.append(p[mask].sum()/fundamental)
detail=10*np.log10(powers[0]/powers[1]);assert detail>3,(powers,detail)
print(f'PASS: full-app Auto reduces nonharmonic high-note alias energy by {reduction:.2f} dB versus Legacy, retains {detail:.2f} dB more low-note upper-partial energy (harmonics190..300); all A/B recordings dual mono, unclipped and end in exact silence.')
