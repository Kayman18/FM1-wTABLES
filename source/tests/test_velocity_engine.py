"""Audible engine gain, MIDI routing, fixed mode and velocity-zero note-off."""
from pathlib import Path
import ctypes,numpy as np
R=Path(__file__).resolve().parents[1]
exec((R/'tests/compare_bipolar_bypass.py').read_text().split('libs=')[0])
lib=ctypes.CDLL(str(R/'native/libtest20engine.so'))
lib.core_set.argtypes=[ctypes.POINTER(Settings)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint32,ctypes.c_uint32]
lib.core_midi.argtypes=[ctypes.c_uint,ctypes.c_uint8,ctypes.c_uint8,ctypes.c_uint8]
pcm=(ctypes.c_int32*512)()
def render(vel,mode,amount,curve=0,tracking=False,port=2,unison=0):
 lib.core_init();c=Settings();c.note=48;c.pos=35;c.bank=2;c.level=60;c.env=Env(0,0,127,0);c.env2=Env(0,0,127,0);c.filter=Filter(0,96,0,100)
 c.unison=unison;c.quality=1;c.velocitymode=mode;c.velocityamount=amount;c.velocitycurve=curve
 if tracking:c.tracking=Tracking(50,30,60,20);c.filter=Filter(1,60,40,100)
 lib.core_set(ctypes.byref(c))
 for _ in range(16):lib.core_render(pcm,256,4096)
 lib.core_midi(port,0x95,60,vel);out=[]
 for _ in range(40):lib.core_render(pcm,256,4096);out.append(np.array(pcm,dtype=np.float64))
 assert lib.core_note_held(port,5,60)==1
 lib.core_midi(port,0x95,60,0);assert lib.core_note_held(port,5,60)==0
 return np.concatenate(out[10:])
reference=render(127,0,100);assert abs(reference).max()>1000
worst=0
for vel in [1,8,32,64,96,127]:
 for amount in [0,25,50,75,100]:
  signal=render(vel,1,amount);gain=1+(vel/128-1)*(amount*.01)
  err=abs(signal-reference*gain).max();worst=max(worst,err);assert err<3,(vel,amount,err)
for mode in range(2):
 for tracking in [False,True]:
  full=render(127,mode,75,0,tracking)
  for vel in [1,32,64,127]:assert np.array_equal(render(vel,mode,75,1,tracking),full)
print(f'PASS: engine velocity/amount amplitude ratios (max quantization error {worst:.2f} signed24 counts); USB/TRS-style per-channel note ownership, Fixed mapping including velocity tracking/filter routes, velocity-zero note-off in both response modes.')

mapper=ctypes.CDLL(str(R/'native/libpwvelocity.so'))
mapper.pw_velocity_map.argtypes=[ctypes.c_uint]*3
for mode in range(2):
 for tracking in [False,True]:
  for port in range(3):
   for unison in range(3):
    for curve in [2,3]:
     for vel in [1,16,32,64,96,127]:
      mapped=mapper.pw_velocity_map(vel,1,curve)
      assert np.array_equal(render(vel,mode,75,curve,tracking,port,unison),render(mapped,mode,75,0,tracking,port,unison)),(mode,tracking,port,unison,curve,vel)
print('PASS: Hard/Soft engine PCM equals independently pre-mapped Linear input across both amplitude modes, tracking/filter on/off, all ports, 1/2/4-voice unison, and low/mid/high velocities; zero-velocity note-off preserved.')

def held_or_panel(curve,panel=False):
 lib.core_init();c=Settings();c.note=48;c.pos=35;c.bank=2;c.level=60;c.env=Env(0,0,127,0);c.env2=Env(0,0,127,0);c.filter=Filter(1,80,25,100)
 c.quality=1;c.velocitymode=1;c.velocityamount=100;c.tracking=Tracking(70,0,50,0)
 lib.core_set(ctypes.byref(c))
 for _ in range(16):lib.core_render(pcm,256,4096)
 if not panel:lib.core_midi(0,0x90,60,64)
 c.velocitycurve=curve;lib.core_set(ctypes.byref(c))
 if panel:lib.core_keys(1<<7)
 out=[]
 for _ in range(20):lib.core_render(pcm,256,4096);out.append(bytes(pcm))
 return b''.join(out)
lib.core_keys.argtypes=[ctypes.c_uint32]
for panel in [False,True]:
 baseline=held_or_panel(0,panel)
 for curve in [1,2,3]:assert held_or_panel(curve,panel)==baseline
print('PASS: curve changes preserve held-note PCM and do not alter panel-key velocity.')
