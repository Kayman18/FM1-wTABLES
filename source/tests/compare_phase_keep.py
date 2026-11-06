"""Compare TEST11 default-phase PCM with the bundled TEST10 engine baseline, including enabled filters."""
import ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Env(ctypes.Structure):_fields_=[('a',ctypes.c_uint8),('d',ctypes.c_uint8),('s',ctypes.c_uint8),('r',ctypes.c_uint8)]
class Filter(ctypes.Structure):_fields_=[('mode',ctypes.c_uint8),('cutoff',ctypes.c_uint8),('res',ctypes.c_uint8),('mix',ctypes.c_uint8)]
class LFO(ctypes.Structure):_fields_=[('rate',ctypes.c_int8),('wave',ctypes.c_uint8),('cutoff',ctypes.c_uint8),('pitch',ctypes.c_uint8)]
class Tracking(ctypes.Structure):_fields_=[('vw',ctypes.c_int8),('pw',ctypes.c_int8),('vc',ctypes.c_int8),('pc',ctypes.c_int8)]
class Settings(ctypes.Structure):_fields_=[('play',ctypes.c_uint8),('note',ctypes.c_uint8),('pos',ctypes.c_uint8),('bank',ctypes.c_uint8),('level',ctypes.c_uint8),('env',Env),('filter',Filter),('depth',ctypes.c_uint8),('env2',Env),('depth2',ctypes.c_uint8),('lfo',LFO),('wave1',ctypes.c_int8),('wave2',ctypes.c_int8),('tracking',Tracking),('mode',ctypes.c_uint8),('spread',ctypes.c_uint8),('envpitch',ctypes.c_int8),('ashape',ctypes.c_int8),('dshape',ctypes.c_int8),('a2shape',ctypes.c_int8),('d2shape',ctypes.c_int8),('coarse',ctypes.c_int8),('fine',ctypes.c_int8),('octave',ctypes.c_int8),('bendrange',ctypes.c_int8),('quality',ctypes.c_uint8),('chorus',ctypes.c_uint8),('chorusmix',ctypes.c_uint8),('stereo',ctypes.c_uint8),('spreadst',ctypes.c_int8),('unison',ctypes.c_uint8),('detune',ctypes.c_uint8),('wavespread',ctypes.c_int8),('delaymode',ctypes.c_uint8),('delayfb',ctypes.c_uint8),('delaymix',ctypes.c_uint8),('delayms',ctypes.c_uint16),('velocitymode',ctypes.c_uint8),('velocityamount',ctypes.c_uint8),('velocitycurve',ctypes.c_uint8),('trigger_mode',ctypes.c_uint8),('lfopitchcents',ctypes.c_int16),('chord_shape',ctypes.c_uint8),('chord_scale',ctypes.c_uint8),('chord_key',ctypes.c_int8),('chord_inversion',ctypes.c_uint8),('break_time_ms',ctypes.c_uint16),('break_direction',ctypes.c_uint8),('sustain_mode',ctypes.c_uint8),('voice_limit',ctypes.c_uint8),('sustain_hold',ctypes.c_uint8),('arp_bpm',ctypes.c_uint16),('arp_mode',ctypes.c_uint8),('arp_octaves',ctypes.c_uint8),('arp_gate_percent',ctypes.c_uint8)]
libs=[ctypes.CDLL(str(ROOT/'native'/f'lib{name}.so')) for name in ['test10ref','test11engine']]
for lib in libs:
 lib.core_set.argtypes=[ctypes.POINTER(Settings)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint32,ctypes.c_uint32];lib.core_midi.argtypes=[ctypes.c_uint,ctypes.c_uint8,ctypes.c_uint8,ctypes.c_uint8];lib.core_keys.argtypes=[ctypes.c_uint32];lib.core_init()
rng=random.Random(126105);buffers=[(ctypes.c_int32*512)() for _ in libs]
for block in range(3000):
 c=Settings(block%9==0,rng.randrange(24,84),rng.randrange(101),rng.randrange(8),rng.randrange(101),Env(20,40,80,30),Filter(block%4,rng.randrange(128),rng.randrange(101),rng.randrange(101)),rng.randrange(128),Env(rng.randrange(128),rng.randrange(128),rng.randrange(128),rng.randrange(128)),rng.randrange(128),LFO(rng.randrange(-100,101),rng.randrange(101),rng.randrange(97),rng.randrange(101)),rng.randrange(-100,101),rng.randrange(-100,101),Tracking(rng.randrange(-100,101),rng.randrange(-100,101),rng.randrange(-127,128),rng.randrange(-100,101)),0,0)
 port=rng.choice([0,2]);status=rng.choice([0x90,0x80]);note=rng.randrange(24,84);keys=rng.getrandbits(27) if block%20==0 else 0
 master=rng.randrange(4097)
 for lib,out in zip(libs,buffers):
  lib.core_set(ctypes.byref(c));lib.core_keys(keys);lib.core_midi(port,status,note,100);lib.core_render(out,256,master)
 assert bytes(buffers[0])==bytes(buffers[1]),block
print('PASS: TEST11 default-phase PCM is bit-identical to TEST10 across 3000 randomized blocks, including all tracking/envelope/LFO routes and all filter modes, mix/resonance changes, notes, chords and voice stealing.')
