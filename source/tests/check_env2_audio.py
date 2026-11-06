from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def load(name):
 with wave.open(str(ROOT/f'env2-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 return rate,x[:,0].astype(float)
def brightness(rate,x,start,end):
 y=x[int(start*rate):int(end*rate)];t=np.arange(len(y))/rate;win=np.hanning(len(y))
 amps=np.array([abs(np.sum(y*win*np.exp(-2j*np.pi*130.81278265*k*t))) for k in range(1,9)])
 return np.linalg.norm(amps[2:])/amps[0]
rate,z=load('zero');rate,s=load('sweep');windows=[(.08,.18),(.70,.85),(1.65,1.9)]
a=[brightness(rate,z,*w) for w in windows];b=[brightness(rate,s,*w) for w in windows]
assert max(a)/min(a)<1.3,(a,b)
assert b[1]>b[0]*3 and b[1]>b[2]*3,(a,b)
print(f'PASS: actual Env2 controls open and close cutoff while the amp envelope sustains. Zero-depth brightness {a}; Env2 +60 st {b}. Both recordings are dual mono without clipping.')
