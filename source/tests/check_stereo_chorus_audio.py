import wave,numpy as np
from pathlib import Path
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(n):
 with wave.open(str(r/f'stereo-{n}.wav')) as f:
  sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert abs(x).max()<32767;assert not np.any(x[-1000:]);return sr,x
for n in ['dry','restored']:
 sr,x=read(n);assert np.array_equal(x[:,0],x[:,1])
ratios=[]
for n in ['chorus1','chorus2']:
 sr,x=read(n);x=x[int(.2*sr):int(3.8*sr)];side=np.mean((x[:,0]-x[:,1])**2);mid=np.mean((x[:,0]+x[:,1])**2);ratio=side/mid;assert ratio>.01,(n,ratio);ratios.append(ratio)
def partials(n):
 sr,x=read(n);x=x[int(.3*sr):int(1.3*sr)];f=np.fft.rfftfreq(len(x),1/sr);p=abs(np.fft.rfft(x*np.hanning(len(x))[:,None],axis=0))**2
 return [p[abs(f-440*2**((note-69)/12))<3].sum(axis=0) for note in [48,55,61,68]]
v=partials('voice');assert v[0][0]>v[0][1]*1000 and v[3][1]>v[3][0]*1000,v
p=partials('pairs');assert abs(p[0][0]/p[0][1]-3)<.05
# Pairs at negative spread: slots0/1 at p=.25/.75; slots2/3 at0/1.
assert p[2][0]>p[2][1]*1000 and p[3][1]>p[3][0]*1000,p
q=partials('pitch');assert abs(q[0][0]/q[0][1]-1)<.02;assert abs(q[3][1]/q[3][0]-3.5)<.08,q
print(f'PASS: full-app two chorus modes produce stereo side/mid power {ratios}; Voice, reversed Pairs and Pitch distribute each chord fundamental as recovered; Mono/off restores exact dual mono. All recordings unclipped and end in exact silence.')
