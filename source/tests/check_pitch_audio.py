from pathlib import Path
import wave,numpy as np
root=Path(__file__).resolve().parents[1]/'validation/test22'
def freq(name):
 with wave.open(str(root/f'pitch-{name}.wav')) as f:
  rate=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert abs(x.astype(int)).max()<32767
 y=x[:,0].astype(float);i=np.where((y[:-1]<=0)&(y[1:]>0))[0];z=(i-y[i]/(y[i+1]-y[i]))/rate
 t=(z[:-1]+z[1:])/2;v=1/np.diff(z);keep=(t>.06)&(t<1.95)
 return t[keep],v[keep]
t,z=freq('zero');assert np.std(z)<.05,(z.min(),z.max())
t,u=freq('up');assert 245<u.max()<263;assert abs(u[t>1.65].mean()-130.813)<.2
t,d=freq('down');assert 64<d.min()<70;assert abs(d[t>1.65].mean()-130.813)<.2
t,c=freq('combined');assert c[t<1.2].max()>175;assert c[t>1.5].min()<125 and c[t>1.5].max()>137
print(f'PASS: full-app signed pitch audio, zero {z.mean():.3f} Hz, octave-up peak {u.max():.3f} Hz, octave-down minimum {d.min():.3f} Hz; combined Env2/LFO vibrato, dual mono and no clipping.')
