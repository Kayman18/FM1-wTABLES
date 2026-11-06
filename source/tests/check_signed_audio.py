import wave
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(n):
 with wave.open(str(r/f'signed-filter-{n}.wav')) as f:sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert abs(x).max()<32767 and abs(x).max()>500;assert np.array_equal(x[:,0],x[:,1]);assert not x[-sr//3:].any();return sr,x[:,0]
def brightness(x,sr,t):
 a=x[int(t*sr):int((t+.12)*sr)];p=abs(np.fft.rfft(a*np.hanning(len(a))))**2;freq=np.fft.rfftfreq(len(a),1/sr)
 return p[freq>1000].sum()/p[(freq>80)&(freq<700)].sum()
sr,d=read('down');_,u=read('up');de=brightness(d,sr,.1);dl=brightness(d,sr,1);ue=brightness(u,sr,.1);ul=brightness(u,sr,1)
assert dl<de*.5,(de,dl);assert ul>ue*2,(ue,ul)
print(f'PASS: actual encoder/MIDI recordings give downward cutoff brightness {de:.5g}->{dl:.5g}, upward {ue:.5g}->{ul:.5g}; dual mono, unclipped, release to exact silence.')
