from pathlib import Path
import wave,numpy as np
root=Path(__file__).resolve().parents[1]
with wave.open(str(root/'validation/test22/keyboard-midi.wav'),'rb') as w:
 rate=w.getframerate();x=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
assert np.array_equal(x[:,0],x[:,1]);x=x[:,0].astype(float)
assert max(abs(x))<32767
# Script: physical C4, USB C4, TRS E4, each held one second, with one second gaps.
for onset,note in [(.2,60),(2.204,60),(4.208,64)]:
 expected=440*2**((note-69)/12)
 a=x[int((onset+.65)*rate):int((onset+.95)*rate)]
 fft=abs(np.fft.rfft(a*np.hanning(len(a)),131072));freqs=np.fft.rfftfreq(131072,1/rate)
 peak=freqs[np.argmax(fft)]
 assert abs(peak-expected)<1,(note,peak,expected)
 assert np.sqrt(np.mean(a*a))>100
 # A long release must have ended within the following one-second gap.
 tail=x[int((onset+1.65)*rate):int((onset+1.95)*rate)]
 assert np.max(abs(tail))==0,(note,np.max(abs(tail)))
print('PASS: host physical key C4, USB MIDI C4 and TRS MIDI E4 have the expected pitch, audible held notes, release tails ending in exact silence, dual-mono output and no clipping.')
