import ctypes
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
exec((ROOT/'tests/compare_dual45.py').read_text().split('libs=')[0])
libs=[ctypes.CDLL(str(ROOT/'native'/n)) for n in ['libtest44ref.so','libtest45thread.so']]
buffers=[(ctypes.c_int32*2048)() for _ in libs]
for lib in libs:
 lib.pw_preset_get.argtypes=[ctypes.c_uint,ctypes.POINTER(Settings)];lib.core_set.argtypes=[ctypes.POINTER(Settings)];lib.core_render.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint,ctypes.c_uint]
for chorus in (1,2):
 for delay in (1,2):
  for style in (0,1):
   configs=[]
   for lib in libs:
    lib.core_init();c=Settings();lib.pw_preset_get(10,ctypes.byref(c));c.voice_limit=8;c.chorus=chorus;c.chorusmix=100;c.delaymode=delay
    c.delay_style=style;c.delay_return_db=-6;c.delaymix=65;c.delayfb=80;c.delayms=450;lib.core_set(ctypes.byref(c));lib.core_midi(0,0x90,60,100);configs.append(c)
   for block in range(1200):
    n=[1,63,64,65,127,256,511,1024][block%8]
    for lib,c,out in zip(libs,configs,buffers):
     if block==400:c.chorusmix=45;c.delayms=100
     if block==700:c.chorusmix=100;c.delayms=550
     if block==900:lib.core_midi(0,0x80,60,0)
     lib.core_set(ctypes.byref(c));lib.core_render(out,n,4096)
    assert bytes(buffers[0])==bytes(buffers[1]),(chorus,delay,style,block)
print('PASS: both chorus modes, stereo/ping-pong delays and both delay styles match TEST44 exactly across steady controls, mix/time changes, release tails and variable block lengths.')
