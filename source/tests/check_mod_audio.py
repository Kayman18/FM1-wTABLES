from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def load(name):
 with wave.open(str(ROOT/f'env-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 return rate,x[:,0].astype(float)
def brightness(rate,x,start,end):
 y=x[int(start*rate):int(end*rate)];t=np.arange(len(y))/rate;win=np.hanning(len(y))
 amps=np.array([abs(np.sum(y*win*np.exp(-2j*np.pi*130.81278265*k*t))) for k in range(1,9)])
 return np.linalg.norm(amps[2:])/amps[0]
rate,z=load('zero');rate,s=load('sweep')
windows=[(.08,.18),(.70,.85),(1.65,1.9)]
zero=[brightness(rate,z,*w) for w in windows];sweep=[brightness(rate,s,*w) for w in windows]
assert max(zero)/min(zero)<1.3,(zero,sweep)
assert sweep[1]>sweep[0]*3 and sweep[1]>sweep[2]*3,(zero,sweep)
print(f'PASS: actual FX/SEL encoder depth control changes note brightness over its ADSR. Zero-depth brightness {zero}; +60-semitone sweep {sweep}. Peak is >3x early/late brightness, while zero depth varies by less than 30% (amplitude-envelope/filter transients and PCM quantization remain). Both recordings are dual mono without clipping.')
