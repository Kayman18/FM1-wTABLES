import wave
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(n):
 with wave.open(str(r/f'bipolar-{n}.wav')) as f:sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert np.array_equal(x[:,0],x[:,1]) and abs(x).max()<32767 and not x[-sr//2:].any();return sr,x[:,0]
def freq(x,sr,a,b):
 y=x[int(a*sr):int(b*sr)];i=np.flatnonzero((y[:-1]<=0)&(y[1:]>0));t=(i-y[i]/(y[i+1]-y[i]))/sr;return 1/np.diff(t)
sr,x=read('pitch');low=np.median(freq(x,sr,1.3,2.2));release=np.median(freq(x,sr,2.7,3.3));assert abs(low-65.406)<.3 and abs(release-130.813)<.5,(low,release)
sr,x=read('lfo');f=freq(x,sr,.4,2.8);assert np.percentile(f,5)<75 and np.percentile(f,95)>230
print(f'PASS: full-app bipolar Env2 pitch sustains {low:.2f} Hz and releases to {release:.2f} Hz; signed full-depth LFO spans {np.percentile(f,5):.2f}..{np.percentile(f,95):.2f} Hz; dual mono, unclipped and exact ending silence.')
