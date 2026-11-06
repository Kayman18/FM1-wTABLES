from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test35'
def audio(name):
 with wave.open(str(r/(name+'.wav'))) as w:sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(a[:,0],a[:,1]) and np.max(abs(a.astype(int)))<32767 and not a[-1000:].any()
 return sr,a[:,0]
def powers(x,sr,notes):
 x=x.astype(float)*np.hanning(len(x));t=np.arange(len(x))/sr
 return np.array([abs(np.dot(x,np.exp(-2j*np.pi*440*2**((n-69)/12)*t))) for n in notes])
full={'sixth-oct':[48,52,55,57,60],'seventh-oct':[48,52,55,59,60],'ninth':[48,52,55,59,62]}
for name,notes in full.items():
 sr,a=audio('five-'+name+'-direct');p=powers(a[int(.1*sr):int(.8*sr)],sr,notes);assert p[2]<min(p[[0,1,3,4]])*.02,(name,p)
 assert min(p[[0,1,3,4]])>1000,(name,p)
for name,notes in {**{f'five-{n}-arp':v*2 for n,v in full.items()},'five-omit':[48,52,55,59,60,64,67,71,72]*2}.items():
 sr,a=audio(name);found=[];candidates=sorted(set(notes))
 for k in range(len(notes)):
  p=powers(a[int((k*.25+.035)*sr):int((k*.25+.11)*sr)],sr,candidates);assert max(p)>10
  found.append(candidates[np.argmax(p)])
  assert not a[int((k*.25+.18)*sr):int((k*.25+.22)*sr)].any(),(name,k,'gate gap')
 assert found==notes,(name,found,notes)
print('PASS: five-note host audio: 12 expected direct-voicing pitches with omitted middle member, 48 complete/arpeggiated pitches including octave omission, gate gaps, no clipping and exact final silence.')
