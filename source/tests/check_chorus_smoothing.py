"""Check read-position continuity and audio against an independent analytic delay."""
from pathlib import Path
import ctypes,math,numpy as np,json
R=Path(__file__).resolve().parents[1]
L=ctypes.CDLL(str(R/'native/libpwstereochorus.so'))
class State(ctypes.Structure):_fields_=[('ring',ctypes.c_float*1024),('phase',ctypes.c_uint32),('write',ctypes.c_uint32)]
P=ctypes.POINTER(ctypes.c_float)
L.pw_chorus_tick.argtypes=[ctypes.POINTER(State),P,P,ctypes.c_uint,ctypes.c_uint];L.pw_chorus_tick_smooth.argtypes=[ctypes.POINTER(State),P,P,ctypes.c_uint]
L.pw_chorus_offset_smooth.argtypes=[ctypes.c_uint,ctypes.c_uint32];L.pw_chorus_offset_smooth.restype=ctypes.c_float
z=np.load(R/'validation/chorus-tables.npz');results=[]
for mode,inc,slope in [(1,38956,.30562499165534973),(2,91547,.41062498092651367)]:
 phase=(np.arange(132300,dtype=np.uint64)*inc)&0xffffffff;i=phase>>22
 old_offset=z['index44'][mode-1,i]+z['fraction44'][mode-1,i]
 new_offset=np.array([L.pw_chorus_offset_smooth(mode,int(p)) for p in phase])
 old_jump=float(abs(np.diff(old_offset)).max());new_jump=float(abs(np.diff(new_offset)).max());assert new_jump<.009 and old_jump>new_jump*40
 # Independent triangular-delay formula, with all phase bits retained.
 u=phase.astype(float)/4194304.;tri=np.minimum(u,1024-u);expected=256-(2+slope*tri)*44100/48000
 assert np.max(abs(new_offset-expected))<.00005
 for index in range(1024):assert L.pw_chorus_offset_smooth(mode,index<<22)==float(np.float32(z['index44'][mode-1,index]+z['fraction44'][mode-1,index]))
 # A 6 kHz signal makes read-position zipper error measurable, without clipping.
 def signal(n):return float(np.float32(math.sin(2*math.pi*6000*n/44100)))
 old=State();new=State();old.write=new.write=255;old.phase=new.phase=0xfffff000
 for n in range(256):
  for k in range(4):old.ring[n*4+k]=new.ring[n*4+k]=signal(n-256)
 old_errors=[];new_errors=[];inp=(ctypes.c_float*4)();a=(ctypes.c_float*4)();b=(ctypes.c_float*4)()
 for n in range(12000):
  for k in range(4):inp[k]=signal(n)
  L.pw_chorus_tick(ctypes.byref(old),inp,a,mode,44100);L.pw_chorus_tick_smooth(ctypes.byref(new),inp,b,mode)
  assert old.phase==new.phase and old.write==new.write
  for k,off in enumerate([0,0x80000000,0x40000000,0xc0000000]):
   u=((new.phase+off)&0xffffffff)/4194304.;tri=min(u,1024-u);delay=(2+slope*tri)*44100/48000;t=n-delay;t0=math.floor(t);ref=signal(t0)+(signal(t0+1)-signal(t0))*(t-t0)
   if mode==2:ref=(signal(n)+ref)*.7070000171661377
   old_errors.append(float(a[k])-ref);new_errors.append(float(b[k])-ref)
 old_rms=float(np.sqrt(np.mean(np.array(old_errors)**2)));new_rms=float(np.sqrt(np.mean(np.array(new_errors)**2)));assert new_rms<.0001 and old_rms>new_rms*100,(mode,old_rms,new_rms)
 results.append(dict(mode=mode,old_max_delay_step=old_jump,new_max_delay_step=new_jump,old_audio_reference_error_rms=old_rms,new_audio_reference_error_rms=new_rms))
(R/'validation/test22/chorus-smoothing.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS: continuous chorus read motion, phase wrap and original table knots; analytic triangular delay within .00005 sample; 6 kHz read-position error reduced >100x in both modes:',results)
