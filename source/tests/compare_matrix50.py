"""Compare TEST47 external-bank PCM with the TEST46 embedded baseline, including effects and sequencing."""
import ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Env(ctypes.Structure):_fields_=[('a',ctypes.c_uint8),('d',ctypes.c_uint8),('s',ctypes.c_uint8),('r',ctypes.c_uint8)]
class Filter(ctypes.Structure):_fields_=[('mode',ctypes.c_uint8),('cutoff',ctypes.c_uint8),('res',ctypes.c_uint8),('mix',ctypes.c_uint8)]
class LFO(ctypes.Structure):_fields_=[('rate',ctypes.c_int8),('wave',ctypes.c_uint8),('cutoff',ctypes.c_uint8),('pitch',ctypes.c_uint8)]
class Tracking(ctypes.Structure):_fields_=[('vw',ctypes.c_int8),('pw',ctypes.c_int8),('vc',ctypes.c_int8),('pc',ctypes.c_int8)]
class Settings(ctypes.Structure):_fields_=[('play',ctypes.c_uint8),('note',ctypes.c_uint8),('pos',ctypes.c_uint8),('bank',ctypes.c_uint8),('level',ctypes.c_uint8),('env',Env),('filter',Filter),('depth',ctypes.c_uint8),('env2',Env),('depth2',ctypes.c_uint8),('lfo',LFO),('wave1',ctypes.c_int8),('wave2',ctypes.c_int8),('tracking',Tracking),('mode',ctypes.c_uint8),('spread',ctypes.c_uint8),('envpitch',ctypes.c_int8),('ashape',ctypes.c_int8),('dshape',ctypes.c_int8),('a2shape',ctypes.c_int8),('d2shape',ctypes.c_int8),('coarse',ctypes.c_int8),('fine',ctypes.c_int8),('octave',ctypes.c_int8),('bendrange',ctypes.c_int8),('quality',ctypes.c_uint8),('chorus',ctypes.c_uint8),('chorusmix',ctypes.c_uint8),('stereo',ctypes.c_uint8),('spreadst',ctypes.c_int8),('unison',ctypes.c_uint8),('detune',ctypes.c_uint8),('wavespread',ctypes.c_int8),('delaymode',ctypes.c_uint8),('delayfb',ctypes.c_uint8),('delaymix',ctypes.c_uint8),('delayms',ctypes.c_uint16),('velocitymode',ctypes.c_uint8),('velocityamount',ctypes.c_uint8),('velocitycurve',ctypes.c_uint8),('trigger_mode',ctypes.c_uint8),('lfopitchcents',ctypes.c_int16),('chord_shape',ctypes.c_uint8),('chord_scale',ctypes.c_uint8),('chord_key',ctypes.c_int8),('chord_inversion',ctypes.c_uint8),('break_time_ms',ctypes.c_uint16),('break_direction',ctypes.c_uint8),('sustain_mode',ctypes.c_uint8),('voice_limit',ctypes.c_uint8),('sustain_hold',ctypes.c_uint8),('arp_bpm',ctypes.c_uint16),('arp_mode',ctypes.c_uint8),('arp_octaves',ctypes.c_uint8),('arp_gate_percent',ctypes.c_uint8),('arp_clock',ctypes.c_uint8),('gain_db',ctypes.c_int8),('delay_return_db',ctypes.c_int8),('delay_style',ctypes.c_uint8),('unison_count',ctypes.c_uint8),('bloom',ctypes.c_uint8*16)]
class Settings46(Settings):_fields_=[('matrix_source',ctypes.c_uint8*4),('matrix_dest',ctypes.c_uint8*4),('matrix_amount',ctypes.c_int8*4)]
Settings=Settings46
assert ctypes.sizeof(Settings)==108
libs=[ctypes.CDLL(str(ROOT/'native'/f'lib{name}.so')) for name in ['test49ref','test50thread']]
for lib in libs:
 lib.core_set.argtypes=[ctypes.POINTER(Settings)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint32,ctypes.c_uint32];lib.core_midi.argtypes=[ctypes.c_uint,ctypes.c_uint8,ctypes.c_uint8,ctypes.c_uint8];lib.core_keys.argtypes=[ctypes.c_uint32];lib.core_init()
rng=random.Random(126105);buffers=[(ctypes.c_int32*2048)() for _ in libs]
for block in range(5000):
 bank=block//625
 if block%625==0:
  for lib in libs:lib.core_init()
  for _ in range(32):
   for lib in libs:lib.pw_tables_service(bank,1)
 c=Settings(block%9==0,rng.randrange(128),rng.randrange(101),rng.randrange(8),rng.randrange(101),Env(20,40,80,30),Filter(block%4,rng.randrange(128),rng.randrange(101),rng.randrange(101)),rng.randrange(128),Env(rng.randrange(128),rng.randrange(128),rng.randrange(128),rng.randrange(128)),rng.randrange(128),LFO(rng.randrange(-100,101),rng.randrange(101),rng.randrange(97),rng.randrange(101)),rng.randrange(-100,101),rng.randrange(-100,101),Tracking(rng.randrange(-100,101),rng.randrange(-100,101),rng.randrange(-127,128),rng.randrange(-100,101)),rng.randrange(4),rng.randrange(91),rng.randrange(-120,121),rng.randrange(-64,64),rng.randrange(-64,64),rng.randrange(-64,64),rng.randrange(-64,64),rng.randrange(-60,61),rng.randrange(-100,101),rng.randrange(-2,3),rng.randrange(-2,47),rng.randrange(2),rng.randrange(3),rng.randrange(101),rng.randrange(4),rng.randrange(-100,101),rng.randrange(3),rng.randrange(101),rng.randrange(-100,101),rng.randrange(3),rng.randrange(91),rng.randrange(101),rng.randrange(1,551),rng.randrange(2),rng.randrange(101),rng.randrange(4))
 c.bank=bank
 c.gain_db=block%65-40;c.delay_return_db=-(block%41);c.delay_style=block%2;c.quality=block%2;c.arp_clock=(block//200)%3;c.arp_mode=(block//100)%15;c.arp_bpm=40+block%201;c.arp_octaves=1;c.arp_gate_percent=10+block%81;c.sustain_hold=block%2;c.voice_limit=1+block%6;c.unison_count=1+(block//3)%8;c.sustain_mode=block%3;c.break_time_ms=block%1001;c.break_direction=block%3;c.chord_shape=block%15;c.chord_scale=block%9;c.chord_key=block%25-12;c.chord_inversion=block%4;c.trigger_mode=block%2;c.env2.s=rng.randrange(-127,128);c.lfopitchcents=rng.randrange(-1200,1201)
 for row in range(4):
  c.matrix_source[row]=1+(block+row)%11;c.matrix_dest[row]=(block//50+row)%3;c.matrix_amount[row]=(block+row*31)%201-100
 c.depth=rng.randrange(-127,128);c.depth2=rng.randrange(-127,128);c.lfo.wave=rng.randrange(-100,101);c.lfo.cutoff=rng.randrange(-127,128)
 # Exercise Bloom, then arp/direct input, with sample-exact event boundaries.
 c.bloom[0]=int(block>=3000);c.bloom[1]=1+block%16;c.bloom[2]=block%7;c.bloom[3]=block%12;c.bloom[4]=block%5;c.bloom[5]=block%101
 if block>=3000:c.play=1;c.arp_mode=0
 if block%31==0:libs[1].core_parallel_set((block//31)%2)
 if block%43==0:libs[1].test_worker_available((block//43)%2)
 port=rng.choice([0,2]);status=rng.choice([0x90,0x80]);note=rng.randrange(128);keys=0
 master=rng.randrange(4097);frames=[1,63,64,65,127,255,256,511,1024][block%9]
 for lib,out in zip(libs,buffers):
  lib.core_set(ctypes.byref(c));lib.core_keys(keys);
  if block%100==0:lib.core_midi(2 if c.arp_clock==2 else 0,0xfa,0,0)
  if block%3==0:lib.core_midi(2 if c.arp_clock==2 else 0,0xf8,0,0)
  lib.core_midi(port,0xb0,64,127 if block%20<10 else 0);lib.core_midi(port,status,note,(block%127)+1);lib.core_render(out,frames,master)
 assert bytes(buffers[0])==bytes(buffers[1]),block
 for slot in range(8):
  for name in ['core_voice_phase','core_voice_note','core_voice_increment','core_voice_delay']:
   assert getattr(libs[0],name)(slot)==getattr(libs[1],name)(slot),(block,slot,name)
print('PASS: TEST50 legacy destinations concurrent-worker/fallback PCM matches TEST49 (all 8 preloaded banks) across 5000 randomized full-range blocks, all voice budgets, effects, chords and arp modes.')
# Fault after a worker has touched state must latch silence without replay.
c.bloom[0]=0;c.play=1;c.arp_mode=0;c.voice_limit=8;c.unison_count=8
lib=libs[1];lib.core_init();lib.test_worker_available(1);lib.core_set(ctypes.byref(c));lib.test_worker_fail();lib.core_render(buffers[1],256,4096)
assert lib.core_worker_fault()==1 and not any(buffers[1][:512])
lib.core_render(buffers[1],256,4096);assert not any(buffers[1][:512])
print('PASS: worker fault latches silence; partially rendered voices are never replayed.')
