from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
hz=.01*np.exp(83*0.06907755136489868)
def fit(name):
 with wave.open(str(ROOT/f'phase-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 y=x[:,0].astype(float);i=np.flatnonzero((y[:-1]<=0)&(y[1:]>0));times=(i-y[i]/(y[i+1]-y[i]))/rate
 freq=1/np.diff(times);t=(times[1:]+times[:-1])*.5;keep=(t>.04)&(t<.55);t=t[keep];freq=freq[keep]
 model=np.column_stack([np.ones(len(t)),np.sin(2*np.pi*hz*t),np.cos(2*np.pi*hz*t)])
 a=np.linalg.lstsq(model,freq,rcond=None)[0];assert np.hypot(a[1],a[2])>7
 return np.arctan2(a[2],a[1])
def distance(a,b):return abs(np.angle(np.exp(1j*(a-b))))
poly=distance(fit('poly1'),fit('poly2'));mono=distance(fit('mono1'),fit('mono2'));off=distance(fit('off1'),fit('off2'))
assert poly<.16 and mono<.16 and off>.5,(poly,mono,off)
print(f'PASS: actual LFO Phase controls restart Poly retriggers and Mono first notes, while Off continues without reset. Measured vibrato phase differences: Poly {poly:.3f}, Mono {mono:.3f}, Off {off:.3f} radians. All recordings dual mono without clipping.')
