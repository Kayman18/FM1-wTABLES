import ctypes
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
exec((ROOT/'tests/compare_dual44.py').read_text().split('libs=')[0])
libs=[ctypes.CDLL(str(ROOT/'native'/n)) for n in ['libtest43ref.so','libtest44thread.so']]
buffers=[(ctypes.c_int32*512)() for _ in libs]
for lib in libs:
 lib.pw_preset_get.argtypes=[ctypes.c_uint,ctypes.POINTER(Settings)]
 lib.pw_preset_apply.argtypes=[ctypes.c_uint,ctypes.POINTER(Settings)]
 lib.core_set.argtypes=[ctypes.POINTER(Settings)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint,ctypes.c_uint]
for preset in range(17):
 for budget in range(5,9):
  configs=[]
  for lib in libs:
   lib.core_init();c=Settings();lib.pw_preset_get(preset,ctypes.byref(c));c.voice_limit=budget;c.bloom[0]=1;c.bloom[5]=35;c.arp_bpm=137;lib.core_set(ctypes.byref(c));c.play=1;lib.core_set(ctypes.byref(c));configs.append(c)
  for block in range(800):
   for lib,c,out in zip(libs,configs,buffers):
    if block in (173,411):lib.pw_preset_apply((preset+block)%17,ctypes.byref(c))
    if block%99==0:c.octave=(block//99)%5-2
    lib.core_set(ctypes.byref(c));lib.core_render(out,256,4096)
   assert bytes(buffers[0])==bytes(buffers[1]),(preset,budget,block)
   assert libs[0].core_bloom_step()==libs[1].core_bloom_step()
   assert libs[0].core_bloom_note()==libs[1].core_bloom_note()
print('PASS: 17 presets x voice limits 5–8 x 800 blocks: TEST43-exact PCM and sequence state, including live instrument and octave changes.')
