from pathlib import Path
import ctypes,json
R=Path(__file__).resolve().parents[1];p=R/'tests/compare_stereo_bypass.py';source=p.read_text();ns={'__file__':str(p)};exec(source[:source.index('libs=')],ns)
S,E=ns['Settings'],ns['Env'];results=[]
for name in ['test16crunchref','test17engine']:
 lib=ctypes.CDLL(str(R/'native'/f'lib{name}.so'));lib.core_set.argtypes=[ctypes.POINTER(S)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint32,ctypes.c_uint32]
 lib.core_init();c=S();c.note=48;c.level=100;c.env=E(0,0,127,0);c.quality=1;c.chorus=2;c.chorusmix=100;lib.core_set(ctypes.byref(c))
 for ch in range(4):lib.core_midi(0,0x90|ch,48,127)
 out=(ctypes.c_int32*512)();peak=clipped=0
 for block in range(400):
  lib.core_render(out,256,4096);peak=max(peak,max(abs(x) for x in out));clipped+=sum(x in (8388607,-8388608) for x in out)
 results.append(dict(engine=name,peak=peak,clipped_samples=clipped))
assert results[0]['clipped_samples']>20000 and results[1]['clipped_samples']==0
(R/'validation/test22/chorus-crunch-reproduction.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS: captured TEST16 Mode2 distortion reproduced versus TEST17 at identical four-note/full-level settings:',results)
