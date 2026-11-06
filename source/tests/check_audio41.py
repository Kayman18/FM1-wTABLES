from pathlib import Path
import json,hashlib,wave,subprocess
import numpy as np
r=Path(__file__).resolve().parents[1];v=r/'validation/test41';rows=json.loads((r/'presets/fm1-presets.json').read_text());metrics=[];clips=[];hashes=set()
for i,row in enumerate(rows,1):
 p=v/f'preset-{i:02}.raw';a=np.fromfile(p,dtype='<i4').reshape(-1,2)/8388608.
 assert a.shape==(939330,2) and np.max(np.abs(a))<1 and np.max(np.abs(a))>.001 and not a[-4410:].any()
 with wave.open(str(v/f'preset41-{i:02}.wav')) as f:
  assert f.getsampwidth()==2
  h=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2')/32768.
 assert np.max(np.abs(h))<1 and np.max(np.abs(h))>.001
 digest=hashlib.sha256(p.read_bytes()).hexdigest();hashes.add(digest)
 metrics.append({'index':i,'name':row['name'],'peak':float(np.max(np.abs(a))),'rms':float(np.sqrt(np.mean(a*a))),'dc':float(np.mean(a)),'host_peak':float(np.max(np.abs(h))),'sha256':digest})
 # Four seconds of single-note playback, late enough to hear slower attacks.
 clip=a[int(3.1*44100):int(7.1*44100)].copy();fade=np.linspace(0,1,441)[:,None];clip[:441]*=fade;clip[-441:]*=fade[::-1];clips.extend([clip,np.zeros((11025,2))])
assert len(hashes)==16
(v/'preset-audio-metrics.json').write_text(json.dumps(metrics,indent=2)+'\n')
out=r.parent/'deliverables';out.mkdir(exist_ok=True);wav=v/'demo.wav'
with wave.open(str(wav),'wb') as f:
 f.setnchannels(2);f.setsampwidth(2);f.setframerate(44100);f.writeframes((np.concatenate(clips)*32767).astype('<i2').tobytes())
subprocess.run(['ffmpeg','-y','-loglevel','error','-i',str(wav),'-codec:a','libmp3lame','-b:a','192k',str(out/'FM1-PolyWavetable-TEST41-Preset-Demo.mp3')],check=True)
(v/'demo-tracklist.txt').write_text('\n'.join(f'{(i*4.25)//60:02.0f}:{(i*4.25)%60:05.2f}  {row["name"]}' for i,row in enumerate(rows))+'\n')
print('PASS: 16 distinct preset renders and 16 host PLAY recordings; nonzero audio, no clipping, panic silence. Demo: 68 seconds, native relative levels.')
