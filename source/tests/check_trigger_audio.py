from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test24'
def read(name):
 with wave.open(str(r/(name+'.wav'))) as w:
  assert w.getnchannels()==2 and w.getsampwidth()==2 and w.getframerate()==44100
  x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert abs(x.astype(np.int32)).max()<32767 and abs(x).max()>100
 assert np.array_equal(x[:,0],x[:,1]) and not np.any(x[-4410:])
 return x
short=read('triggered-tap');held=read('triggered-held');gated=read('gated-tap')
assert np.array_equal(short,held),'Triggered short tap and held key must have identical PCM'
ends=[np.where(abs(x[:,0])>2)[0][-1]/44100 for x in [gated,short]]
assert .06<ends[0]<.09 and .15<ends[1]<.18
assert np.any(short[4410:6000]) and not np.any(gated[4410:6000])
print(f'PASS: triggered full-app selector: 10ms MIDI tap equals held-note PCM exactly; gated tap ends at {ends[0]*1000:.1f}ms, triggered at {ends[1]*1000:.1f}ms; dual mono, no clipping and exact ending silence.')
