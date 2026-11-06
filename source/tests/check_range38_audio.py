from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test38'
def audio(name):
 with wave.open(str(r/(name+'.wav'))) as w:sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert sr==44100 and np.array_equal(a[:,0],a[:,1]) and abs(a).max()<32767 and not a[-1000:].any(),name
 return a[:,0],sr
def spectrum(x,sr):return np.fft.rfftfreq(len(x),1/sr),abs(np.fft.rfft(x*np.hanning(len(x))))
for note in [0,12,23,24,83,84,108,120,127]:
 x,sr=audio(f'range38-note-{note}');x=x[int(.2*sr):int(3.8*sr)];f,p=spectrum(x,sr);peak=f[p.argmax()];expected=440*2**((note-69)/12)
 assert abs(peak-expected)<.35,(note,peak,expected)
 assert p.max()>10000,(note,p.max())
for name,notes in [('low-chord',[3,6]),('high-chord',[120,124,127])]:
 x,sr=audio('range38-'+name);x=x[int(.2*sr):int((3.8 if name=='low-chord' else 1.8)*sr)];f,p=spectrum(x,sr)
 for note in notes:
  expected=440*2**((note-69)/12);assert p[abs(f-expected)<.7].max()>p.max()*.1,(name,note)
x,sr=audio('range38-arp')
for k,note in enumerate([115,127,None,115,127,None]):
 a=x[int((k*.25+.025)*sr):int((k*.25+.11)*sr)]
 if note is None:assert not a.any(),k
 else:
  f,p=spectrum(a,sr);expected=440*2**((note-69)/12);assert abs(f[p.argmax()]-expected)<15,(k,f[p.argmax()],expected)
 assert not x[int((k*.25+.18)*sr):int((k*.25+.22)*sr)].any(),(k,'gate gap')
print('PASS: TEST38 host audio: nine MIDI pitches spanning 0..127, below-tonic and upper-clipped chords, upper arpeggio pitches/rests without wrap, gate gaps, no clipping and exact stop silence.')
