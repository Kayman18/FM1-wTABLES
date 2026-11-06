from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test37'
def audio(name):
 with wave.open(str(r/(name+'.wav'))) as w:sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2).astype(float)
 assert sr==44100 and np.max(abs(a))<32767 and not a[-1000:].any(),name
 return a,sr
root=440*2**((60-69)/12)
for count in range(1,9):
 a,sr=audio(f'poly37-unison-{count}');a=a[int(.5*sr):int(3.5*sr)];f=np.fft.rfftfreq(len(a),1/sr);p=abs(np.fft.rfft(a*np.hanning(len(a))[:,None],axis=0))**2
 values=[]
 for k in range(count):
  units=0 if count==1 else (k*2*21800//(count-1))-21800
  hz=root*2**(units/524288);v=p[abs(f-hz)<.65].sum(axis=0);assert v.sum()>p.sum()*.015,(count,k,v);values.append(v)
 if count==8:assert values[0][0]>values[0][1]*500 and values[-1][1]>values[-1][0]*500,values
notes={'sixth':[48,52,55,57,60],'seventh':[48,52,55,59,60],'ninth':[48,52,55,59,62],'eight':[48,50,52,53,55,57,59,60]}
for name,expected in notes.items():
 a,sr=audio('poly37-'+name);x=a[int(.1*sr):int(.8*sr),0];x=x*np.hanning(len(x));t=np.arange(len(x))/sr
 strengths=[abs(np.dot(x,np.exp(-2j*np.pi*440*2**((n-69)/12)*t))) for n in expected]
 assert min(strengths)>10000 and min(strengths)>max(strengths)*.25,(name,strengths)
print('PASS: TEST37 host audio: all 36 detuned components across unison counts 1..8, eight-slot stereo endpoints, all 15 complete five-note chord pitches, eight independent polyphonic pitches, no clipping and exact final silence.')
