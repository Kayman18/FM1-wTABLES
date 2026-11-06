from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test39'
scales=[list(range(12)),[0,2,4,5,7,9,11],[0,2,3,5,7,8,10],[0,2,5,7,9],[0,3,5,7,10],[0,2,3,5,7,8,11],[0,2,4,6,8,10]]
for scale,pcs in enumerate(scales):
 with wave.open(str(r/f'bloom39-scale-{scale}.wav')) as w:
  sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert sr==44100 and np.array_equal(a[:,0],a[:,1]) and abs(a.astype(int)).max()<32767
 x=a[:,0].astype(float);start=np.flatnonzero(abs(x)>10)[0]/sr
 assert not x[-1000:].any()
 for k in range(32):
  raw=(k%16*7)%25;pc=min(pcs,key=lambda p:abs(p-raw%12));note=48+raw//12*12+pc
  expected=440*2**((note-69)/12)
  y=x[int((start+k*.25+.025)*sr):int((start+k*.25+.105)*sr)]
  p=abs(np.fft.rfft(y*np.hanning(len(y)),65536));f=np.fft.rfftfreq(65536,1/sr);peak=f[p.argmax()]
  assert abs(peak-expected)<4,(scale,k,note,peak,expected)
  assert not x[int((start+k*.25+.19)*sr):int((start+k*.25+.22)*sr)].any(),(scale,k,'gate')
print('PASS: Bloom host audio: 224 sequenced pitches across all seven scales, two complete 16-step cycles each, gate gaps, no clipping and exact stop silence.')
