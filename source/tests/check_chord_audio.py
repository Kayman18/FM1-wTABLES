from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]
with wave.open(str(ROOT/'validation/test22/chord.wav'),'rb') as w:
 rate=w.getframerate();audio=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
assert np.array_equal(audio[:,0],audio[:,1]);x=audio[:,0].astype(float)
assert np.max(abs(x))<32767
notes=[60,62,64,65,67,72]
def amplitudes(start,end):
 y=x[int(start*rate):int(end*rate)];t=np.arange(len(y))/rate
 cols=[]
 for n in notes:
  phase=2*np.pi*440*2**((n-69)/12)*t;cols.extend([np.sin(phase),np.cos(phase)])
 coeff=np.linalg.lstsq(np.array(cols).T,y,rcond=None)[0]
 return np.hypot(coeff[::2],coeff[1::2])
for start,end,expected in [(.6,.9,[60,64,67,72]),(1.5,1.65,[60,67,72]),(2.1,2.3,[60,65,67,72]),(2.8,3.0,[62,65,67,72])]:
 amps=amplitudes(start,end)
 for note,amp in zip(notes,amps):assert amp>7000 if note in expected else amp<10,(start,note,amp,expected)
# All releases have ended after the configured 439ms.
assert np.max(abs(x[int(3.65*rate):]))==0
print('PASS: full host chords contain all four expected pitches simultaneously; one note releases independently; a replacement chord retains its peers; fifth-note stealing removes the oldest held pitch; all tails end in silence. Peak PCM: '+str(int(np.max(abs(x)))))
