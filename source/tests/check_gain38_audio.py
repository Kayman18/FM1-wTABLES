from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test38'
def audio(name):
 with wave.open(str(r/(name+'.wav'))) as w:
  assert w.getframerate()==44100
  a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert abs(a).max()<32767 and not a[-1000:].any(),name
 return a
base=audio('gain36-0')
for db in [-6,-40,24]:
 a=audio(f'gain36-{db}');expected=0 if db==-40 else 10**(db/20)
 assert len(a)==len(base)
 error=abs(a[5000:15000]-base[5000:15000]*expected).max()
 assert error<1+expected,(db,error)
 if db==-40:assert not a.any()
dry=audio('return36-dry');stereo=audio('return36-stereo');ping=audio('return36-pingpong');tap=2206;level=10**(-3/20)
assert abs(dry[:,0]).max()>100 and abs(dry[:,1]).max()==0
for name,a in [('stereo',stereo),('pingpong',ping)]:
 assert np.array_equal(a[:tap],dry[:tap]),name
 for echo in range(1,10):
  expected=dry[:600]*level*.5**(echo-1)
  if name=='pingpong' and echo%2:expected=expected[:,::-1]
  error=abs(a[echo*tap:echo*tap+600]-expected).max()
  assert error<2,(name,echo,error)
print('PASS: TEST36 host audio: -6/+24 dB gain ratios, exact -40 mute, dry retained, nine echoes with recovered -3 dB return and 50% decay, alternating ping-pong lanes; no clipping and final silence.')
