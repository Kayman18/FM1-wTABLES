from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def load(name):
 with wave.open(str(ROOT/f'lfo-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 return rate,x[:,0].astype(float)
def trace(rate,x):
 out=[]
 for start in np.arange(.25,len(x)/rate-.15,.02):
  y=x[int(start*rate):int((start+.12)*rate)];t=np.arange(len(y))/rate;window=np.hanning(len(y))
  amp=np.array([abs(np.sum(y*window*np.exp(-2j*np.pi*130.81278265*k*t))) for k in range(1,10)])
  out.append(np.linalg.norm(amp[2:])/amp[0])
 return np.array(out)
rate,dry=load('dry');rate,wave_audio=load('wave');rate,pitch=load('pitch');rate,filtered=load('filter')
a,b,c=trace(rate,dry),trace(rate,wave_audio),trace(rate,filtered)
assert np.std(a)<.005
assert np.std(b)>.03 and np.std(b)>np.std(a)*10
assert np.percentile(c,95)>np.percentile(c,5)*2
# Pure-sine vibrato recording: interpolate positive-going zero crossings.
idx=np.flatnonzero((pitch[:-1]<=0)&(pitch[1:]>0))
times=(idx-pitch[idx]/(pitch[idx+1]-pitch[idx]))/rate
freq=1/np.diff(times);t=(times[1:]+times[:-1])*.5
keep=(t>.3)&(t<2.9);freq=freq[keep];t=t[keep]
assert np.min(freq)<125 and np.max(freq)>137,(np.min(freq),np.max(freq))
hz=.01*np.exp(83*0.06907755136489868)
model=np.column_stack([np.ones(len(t)),np.sin(2*np.pi*hz*t),np.cos(2*np.pi*hz*t)])
pred=model@np.linalg.lstsq(model,freq,rcond=None)[0]
assert np.corrcoef(freq,pred)[0,1]>.99
print(f'PASS: actual LFO encoders modulate wave brightness (std {np.std(b):.3f}, dry {np.std(a):.5f}), filter brightness ({np.percentile(c,95)/np.percentile(c,5):.2f}x range), and sine pitch ({np.min(freq):.2f}–{np.max(freq):.2f} Hz) at ~{hz:.3f} Hz. All recordings are dual mono without clipping.')
