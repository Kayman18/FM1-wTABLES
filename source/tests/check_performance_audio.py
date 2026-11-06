import wave,numpy as np
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'validation/test22'
def load(n):
 with wave.open(str(root/f'performance-{n}.wav')) as f:
  sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert abs(x.astype(int)).max()<32767
 assert not np.any(x[-1000:]);return sr,x[:,0].astype(float)
def frequency(sr,x,start,end):
 y=x[int(start*sr):int(end*sr)];i=np.where((y[:-1]<=0)&(y[1:]>0))[0];z=i-y[i]/(y[i+1]-y[i]);return np.mean(sr/np.diff(z))
sr,x=load('bend');hz=[frequency(sr,x,a,a+.3) for a in [.2,.95,1.65,2.35]]
for v,e in zip(hz,[130.8128,146.82,116.5409,130.8128]):assert abs(v-e)<.15,(hz,v,e)
sr,x=load('pedal');assert np.sqrt(np.mean(x[int(.6*sr):int(.9*sr)]**2))>100
sr,x=load('tune');tune=frequency(sr,x,.2,.8);assert abs(tune-130.8128*2**(12.5/12))<.25,tune
sr,x=load('octave');octs=[frequency(sr,x,a,a+.25) for a in [.2,.85,1.6,2.4]]
for v,e in zip(octs,[261.6256,261.6256,523.2511,261.6256]):assert abs(v-e)<.2,(octs,v,e)
sr,x=load('range');wide=frequency(sr,x,.2,.8);assert abs(wide-261.61)<.3,wide
print(f'PASS: full-app MIDI bend {hz}, pedal holds after key-up then releases, tuning {tune:.2f} Hz, captured octave {octs}, TRS 12st bend {wide:.2f} Hz; dual mono, no clipping and exact ending silence.')
