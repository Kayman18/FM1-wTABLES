from pathlib import Path
import wave,numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test32'
for name in ['stop','reenable']:
 with wave.open(str(r/f'arp-{name}.wav')) as w:a=np.frombuffer(w.readframes(w.getnframes()),'<i2')
 if name=='stop':assert np.max(abs(a.astype(int)))>500 and not a[-2000:].any()
 else:assert not a.any()
print('PASS: Arp Off stops PLAY audition to exact silence; re-enabling mode does not restart it.')
