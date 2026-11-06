from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test36'
for port in ['usb','trs']:
 with wave.open(str(r/f'clock-{port}.wav')) as w:
  sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]) and np.max(abs(a.astype(int)))<32767 and not a[-1000:].any()
 pitches=[]
 for step in range(8):
  x=a[int((step*.24+.03)*sr):int((step*.24+.10)*sr),0].astype(float);t=np.arange(len(x))/sr;x*=np.hanning(len(x))
  notes=[48,52,55];powers=[abs(np.dot(x,np.exp(-2j*np.pi*440*2**((n-69)/12)*t))) for n in notes]
  assert max(powers)>10,(port,step,'silent note')
  pitches.append(notes[np.argmax(powers)])
  assert not a[int((step*.24+.17)*sr):int((step*.24+.21)*sr)].any(),(port,step,'gate gap')
 assert pitches==[48,52,55,48,52,55,48,52],(port,pitches)
 assert not a[int(2.1*sr):].any(),(port,'stop')
print('PASS: USB and TRS host recordings: 16 expected pitches at incoming 125 BPM, six-tick gates, wrong-port Stop ignored, no clipping and exact post-Stop silence.')
