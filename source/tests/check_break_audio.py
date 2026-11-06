from pathlib import Path
import numpy as np,wave
r=Path(__file__).resolve().parents[1]/'validation/test26'
for name,order in [('up',[48,52,55]),('down',[55,52,48]),('cancel',[48])]:
 with wave.open(str(r/f'break-{name}.wav')) as w:rate=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]);assert np.max(np.abs(a.astype(int)))<32767
 if name=='cancel':assert not a[int(.15*rate):].any();continue
 for start,end,expected in [(.05,.15,order[:1]),(.25,.35,order[:2]),(.45,.7,order)]:
  x=a[int(start*rate):int(end*rate),0].astype(float);t=np.arange(len(x))/rate;cols=[]
  for note in [48,52,55]:
   phase=2*np.pi*440*2**((note-69)/12)*t;cols.extend([np.sin(phase),np.cos(phase)])
  c=np.linalg.lstsq(np.array(cols).T,x,rcond=None)[0];amps=np.hypot(c[::2],c[1::2])
  for note,amp in zip([48,52,55],amps):assert amp>1000 if note in expected else amp<2,(name,start,note,amp)
 assert not a[-1000:].any()
print('PASS: recorded strum audio: Up/Down add the expected pitches at 200ms intervals; an 80ms tap cancels later notes with no resurrection; unclipped dual mono and exact final silence.')
