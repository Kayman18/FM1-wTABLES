from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(name):
 with wave.open(str(r/f'unison-{name}.wav')) as f:sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert abs(x).max()<32767;assert not np.any(x[-1000:]);x=x[int(.5*sr):int(3.5*sr)];fr=np.fft.rfftfreq(len(x),1/sr);p=abs(np.fft.rfft(x*np.hanning(len(x))[:,None],axis=0))**2;return p,fr
root=440*2**((60-69)/12);edge=436*50*1200/524288
for name,offsets in [('single',[0]),('two',[-edge,edge]),('four',[-edge,-edge/3,edge/3,edge])]:
 p,f=read(name);values=[]
 for cents in offsets:
  freq=root*2**(cents/1200);mask=abs(f-freq)<.65;v=p[mask].sum(axis=0);assert v.sum()>p.sum()*.04,(name,cents,v);values.append(v)
 if name in ['two','four']:assert values[0][0]>values[0][1]*500 and values[-1][1]>values[-1][0]*500,(name,values)
a,f=read('wave-zero');b,_=read('wave-wide');a=a.sum(axis=1);b=b.sum(axis=1);a/=a.sum();b/=b.sum();change=np.sum(abs(a-b));assert change>.05,change
print(f'PASS: full-app one/two/four unison spectral components at recovered detunes, balanced endpoint stereo routing, signed wave-spread timbre change {change:.3f}; all recordings unclipped and end in exact silence.')
