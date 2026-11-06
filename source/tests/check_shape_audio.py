import wave,numpy as np
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'validation/test22'
def load(n):
 with wave.open(str(root/f'shape-{n}.wav')) as f:
  sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert abs(x.astype(int)).max()<32767
 assert np.max(abs(x[-1000:].astype(int)))==0
 return sr,x[:,0].astype(float)
def rms(sr,x,start,end):return np.sqrt(np.mean(x[int(sr*start):int(sr*end)]**2))
sr,lo=load('amp-low');_,hi=load('amp-high')
a=rms(sr,lo,.15,.25)/rms(sr,hi,.15,.25);assert a>1.8,a
# At ~1 second into release, high shape retains more amplitude.
b=rms(sr,hi,2.3,2.5)/rms(sr,lo,2.3,2.5);assert b>1.5,b
def hz(n):
 sr,x=load(n);i=np.where((x[:-1]<=0)&(x[1:]>0))[0];z=(i-x[i]/(x[i+1]-x[i]))/sr;t=(z[1:]+z[:-1])/2;f=1/np.diff(z);return f[(t>.15)&(t<.25)].mean()
l,h=hz('env2-low'),hz('env2-high');assert l>h*1.2,(l,h)
print(f'PASS: actual shape controls change amplitude attack ({a:.2f}x), shared decay/release curve ({b:.2f}x release RMS), independent Env2 pitch attack ({l:.2f} vs {h:.2f} Hz); no clipping, dual mono, exact ending silence.')
