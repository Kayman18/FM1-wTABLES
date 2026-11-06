from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test27';audio={};rms={}
for mode in ['legacy','synth','piano']:
 with wave.open(str(r/f'pedal-{mode}.wav')) as w:sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]);assert np.max(abs(a.astype(int)))<32767;assert not a[-1000:].any()
 audio[mode]=a
 def power(start,end):return float(np.sqrt(np.mean(a[int(start*sr):int(end*sr),0].astype(float)**2)))
 rms[mode]=[power(.65,.75),power(.82,.9),power(1.15,1.25)]
 assert rms[mode][0]>500 and rms[mode][2]>500
assert .98<rms['synth'][1]/rms['synth'][0]<1.02
assert rms['piano'][1]<.9*rms['piano'][0]
assert np.array_equal(audio['legacy'],audio['synth']) # at full sustain, same strike velocity
assert not np.array_equal(audio['piano'],audio['synth'])
print('PASS: pedal recordings: Synth holds its full-sustain level, Piano produces a fresh attack; equal-velocity full-sustain Legacy/Synth PCM matches; dual mono, no clipping, pedal-up ends in silence.')
