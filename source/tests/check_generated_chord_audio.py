from pathlib import Path
import wave,numpy as np
root=Path(__file__).resolve().parents[1]/'validation/test25'
for name,expected in [('chord-triad',[48,52,55]),('chord-inversion',[52,55,60])]:
 with wave.open(str(root/(name+'.wav'))) as w:
  rate=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]);assert np.max(np.abs(a.astype(int)))<32767
 x=a[int(.3*rate):int(1*rate),0].astype(float);t=np.arange(len(x))/rate;cols=[];notes=[48,52,55,60]
 for note in notes:
  phase=2*np.pi*440*2**((note-69)/12)*t;cols.extend([np.sin(phase),np.cos(phase)])
 c=np.linalg.lstsq(np.array(cols).T,x,rcond=None)[0];amps=np.hypot(c[::2],c[1::2])
 for note,amp in zip(notes,amps):assert amp>1000 if note in expected else amp<2,(name,note,amp)
 assert not a[-1000:].any()
print('PASS: generated chord PCM: C major 48/52/55 and first inversion 52/55/60, absent pitches rejected, dual mono exact, no clipping, note-off reaches exact silence.')
