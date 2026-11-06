from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(name):
 with wave.open(str(r/f'delay-{name}.wav')) as f:sr=f.getframerate();x=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert abs(x).max()<32767;return sr,x
def energy(x,sr,a,b):return np.mean(x[int(a*sr):int(b*sr)]**2,axis=0)
sr,off=read('off');assert energy(off,sr,0,.03)[0]>100 and energy(off,sr,.04,1).sum()==0
for name,ch in [('stereo',0),('ping',1)]:
 sr,x=read(name);assert not np.any(x[:int(.095*sr)]);e=energy(x,sr,.1,.135);assert e[ch]>100 and e[1-ch]<.01,(name,e)
 assert not np.any(x[int(.15*sr):]),name
sr,x=read('feedback');a=energy(x,sr,.101,.13);b=energy(x,sr,.201,.23);assert a[1]>100 and a[0]<.01 and b[0]>10 and b[1]<.01,(a,b)
# Feedback 50% halves repeat amplitude, hence quarter power.
assert .18<b[0]/a[1]<.32,b[0]/a[1]
sr,x=read('max');assert not np.any(x[:int(.545*sr)]);assert energy(x,sr,.55,.59)[1]>100;assert not np.any(x[int(.62*sr):])
print('PASS: full-app 100ms Stereo/Ping-pong placement, 50% alternating feedback repeats, 550ms maximum time, wet-only onset/no dry leakage, no clipping and finite zero-feedback tails.')
