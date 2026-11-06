from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def load(name):
 with wave.open(str(ROOT/f'wave-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 return rate,x[:,0].astype(float)
def brightness(rate,x,start,end):
 y=x[int(start*rate):int(end*rate)];t=np.arange(len(y))/rate;win=np.hanning(len(y))
 a=np.array([abs(np.sum(y*win*np.exp(-2j*np.pi*130.81278265*k*t))) for k in range(1,13)])
 return np.linalg.norm(a[1:])/a[0]
rate,z=load('zero');rate,p=load('positive');rate,n=load('negative');rate,a=load('amp')
w=[(.08,.18),(.70,.85),(1.65,1.9)]
bz=[brightness(rate,z,*t) for t in w];bp=[brightness(rate,p,*t) for t in w];bn=[brightness(rate,n,*t) for t in w];ba=[brightness(rate,a,*t) for t in w[:2]]
assert max(bz)<.01,bz
assert bp[0]>.1 and bp[1]>.1 and bp[2]<.01,bp
assert bn[1]<bn[2]/3,bn
assert ba[1]<ba[0]/3,ba
assert np.max(abs(a[int(1.65*rate):]))==0
print(f'PASS: actual Wave Mod encoder controls give Env2 upward/downward timbre sweeps and an Amp-driven downward sweep. Zero-depth upper harmonics {bz}; Env2 +100% {bp}; Env2 -100% {bn}; Amp -100% early/peak {ba}. Dual mono, no clipping, amp sustain-zero silence verified.')
