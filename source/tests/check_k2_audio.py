from pathlib import Path
import wave,numpy as np
root=Path(__file__).resolve().parents[1]
with wave.open(str(root/'validation/test22/audition.wav'),'rb') as w:
 rate=w.getframerate();assert w.getsampwidth()==2 and w.getnchannels()==2
 samples=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)[:,0].astype(float)
def spectrum(start,end):
 x=samples[int(start*rate):int(end*rate)];window=np.hanning(len(x));t=np.arange(len(x))/rate
 amps=np.array([abs(np.sum(x*window*np.exp(-2j*np.pi*130.81278265*k*t))) for k in range(1,17)])
 return amps/amps[0]
a=spectrum(.2,.8);b=spectrum(1.25,1.85)
assert max(abs(samples))<32767
assert np.sqrt(np.sum(a[1:]**2))<.01
assert np.sqrt(np.sum(b[1:]**2))>.1
assert np.linalg.norm(a-b)>.1
line=f'PASS: actual host K2 input 0% -> 100% changes the audio; relative upper-harmonic energy {np.linalg.norm(a[1:]):.5f} -> {np.linalg.norm(b[1:]):.5f}; no PCM clipping.\n'
(root/'validation/test22/K2-audio-check.txt').write_text(line);print(line,end='')
