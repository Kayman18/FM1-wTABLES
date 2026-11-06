from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test31'
patterns={'up':[48,52,55,48,52,55,48,52], 'down':[55,52,48,55,52,48,55,52], 'alt':[48,52,55,52,48,52,55,52], 'alt2':[48,52,55,55,52,48,48,52]}
patterns.update({'step-up':[48,52,48,55,48,52,48,55], 'step-down':[55,52,55,48,55,52,55,48], 'as-played':[55,48,52,55,48,52,55,48]})
for mode,expected in patterns.items():
 with wave.open(str(r/f'arp-{mode}.wav')) as w:
  sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]) and np.max(abs(a.astype(int)))<32767 and not a[-1000:].any()
 found=[]
 for step in range(8):
  x=a[int((step*.25+.035)*sr):int((step*.25+.11)*sr),0].astype(float);t=np.arange(len(x))/sr;x*=np.hanning(len(x))
  power=[abs(np.dot(x,np.exp(-2j*np.pi*440*2**((n-69)/12)*t))) for n in [48,52,55]]
  found.append([48,52,55][np.argmax(power)])
  gap=a[int((step*.25+.18)*sr):int((step*.25+.22)*sr)]
  assert not gap.any(),(mode,step,'gate gap')
 assert found==expected,(mode,found,expected)
print('PASS: seven recorded arpeggios match 56 expected pitches at 120 BPM eighth notes, with audible gate gaps, no clipping, dual mono and exact final silence.')
