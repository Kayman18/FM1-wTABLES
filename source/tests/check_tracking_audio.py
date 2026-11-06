from pathlib import Path
import wave,numpy as np
ROOT=Path(__file__).resolve().parents[1]/'validation/test22'
def spectrum(name,note=48):
 with wave.open(str(ROOT/f'tracking-{name}.wav'),'rb') as w:
  rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
 assert np.array_equal(x[:,0],x[:,1]);assert np.max(abs(x.astype(int)))<32767
 y=x[:,0].astype(float);t=np.arange(len(y))/rate;win=np.hanning(len(y));fund=440*2**((note-69)/12)
 a=np.array([abs(np.sum(y*win*np.exp(-2j*np.pi*fund*k*t))) for k in range(1,13)])
 return a/a[0]
def bright(a):return np.linalg.norm(a[1:])
dry=spectrum('dry-soft');soft=spectrum('wave-soft');hard=spectrum('wave-hard');low=spectrum('note-wave-low');high=spectrum('note-wave-high',72)
assert bright(dry)<.01 and bright(low)<.01
assert np.linalg.norm(soft-hard)>.1
assert bright(high)>.05
cs,ch=spectrum('cut-soft'),spectrum('cut-hard')
assert np.linalg.norm(ch[5:])>np.linalg.norm(cs[5:])*1.5
pl,pd,pt=spectrum('pitch-low'),spectrum('pitch-high-dry',60),spectrum('pitch-high',60)
assert bright(pt)>bright(pd)*1.3
assert abs(bright(pt)/bright(pl)-1)<.2
print(f'PASS: actual tracking controls change timbre with MIDI velocity, move wave position across notes, open cutoff with velocity ({np.linalg.norm(ch[5:])/np.linalg.norm(cs[5:]):.2f}x upper-band brightness), and restore relative octave brightness with 100% key tracking ({bright(pt)/bright(pl):.3f}x low-note reference). Spectra normalized to fundamental; all recordings dual mono without clipping.')
