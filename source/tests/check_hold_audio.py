from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test29'
with wave.open(str(r/'hold.wav')) as w:
 sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
def rms(start,end):return float(np.sqrt(np.mean(a[int(start*sr):int(end*sr),0].astype(float)**2)))
assert np.array_equal(a[:,0],a[:,1])
assert np.max(abs(a.astype(int)))<32767
assert rms(.1,.2)>500 and .98<rms(.8,1.1)/rms(.1,.2)<1.02
assert not a[-1000:].any()
print('PASS: host Hold recording sustains after MIDI note-off, K4 Hold Off releases to exact silence, stable sustain level, dual mono and no clipping.')
