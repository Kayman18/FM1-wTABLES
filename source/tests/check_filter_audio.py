from pathlib import Path
import numpy as np,wave
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def load(name):
 with wave.open(str(ROOT/f'filter-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert abs(x.astype(int)).max()<32767
 x=x[:,0].astype(float);t=np.arange(len(x))/rate;window=np.hanning(len(x));fundamental=130.81278265
 return np.array([2*abs(np.sum(x*window*np.exp(-2j*np.pi*fundamental*k*t)))/sum(window) for k in range(1,17)])
dry,lp,bp,hp,res,mix=[load(n) for n in ['dry','lp','bp','hp','resonance','mix-zero']]
assert lp[7]/dry[7] < .3
assert hp[0]/dry[0] < .15
assert bp[3]/dry[3] > bp[0]/dry[0]
assert res[3]/lp[3] > 3
assert np.linalg.norm(mix-dry)/np.linalg.norm(dry)<.005
print(f'PASS: actual FX encoder controls give low-pass treble reduction ({lp[7]/dry[7]:.3f} at harmonic 8), high-pass bass reduction ({hp[0]/dry[0]:.3f} at fundamental), band-pass shaping, resonance boost ({res[3]/lp[3]:.2f}x near cutoff), and dry output at 0% mix. All mode recordings are dual mono without clipping.')
