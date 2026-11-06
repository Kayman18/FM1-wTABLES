"""Compare native C with execution of the original MIPS oscillator slice.
The slice includes unrelated envelope floating-point housekeeping, which is
executed too, but only phase, wave position and oscillator sample are compared.
Unicorn lacks the PIC32 indexed LHX instruction: a hook implements exactly
those four halfword loads. No oscillator arithmetic is replaced by a hook.
"""
import sys,struct,random,ctypes,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
ROOT=Path(__file__).resolve().parents[1]
class Voice(ctypes.Structure):
 _fields_=[('phase',ctypes.c_uint32),('increment',ctypes.c_int32),('position',ctypes.c_int32),('scale',ctypes.c_int32)]
lib=ctypes.CDLL(str(ROOT/'native/libpwcore.so'));lib.pw_core_tick.argtypes=[ctypes.POINTER(Voice),ctypes.POINTER(ctypes.c_int16),ctypes.c_uint32,ctypes.c_uint32];lib.pw_core_tick.restype=ctypes.c_int32
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x200000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
loads={0x9d02a030:(UC_MIPS_REG_T2,UC_MIPS_REG_T2),0x9d02a034:(UC_MIPS_REG_V1,UC_MIPS_REG_A0),0x9d02a038:(UC_MIPS_REG_A3,UC_MIPS_REG_A3),0x9d02a048:(UC_MIPS_REG_V0,UC_MIPS_REG_T3)}
trace=[]
def lhx(u,a,size,_):
 trace.append(a)
 if a==0x9d02a08c:u.emu_stop();return
 if a in loads:
  dest,index=loads[a];address=(u.reg_read(UC_MIPS_REG_V0)+u.reg_read(index))&0x1fffffff
  value=struct.unpack('<h',u.mem_read(address,2))[0];u.reg_write(dest,value&0xffffffff);u.reg_write(UC_MIPS_REG_PC,a+4)
u.hook_add(UC_HOOK_CODE,lhx)
def w(a,v):u.mem_write(a&0x1fffffff,struct.pack('<I',v&0xffffffff))
def ref(phase,inc,position,scale,mip,frames,count):
 trace.clear()
 for reg,value in [(UC_MIPS_REG_S0,0x80010000),(UC_MIPS_REG_FP,0x80020000),(UC_MIPS_REG_S4,0x80030000),(UC_MIPS_REG_S6,0x80040000),(UC_MIPS_REG_SP,0x80050000)]:u.reg_write(reg,value)
 w(0x30000-0x22bc,scale);w(0x10000-0x4c,inc);w(0x10000-0x50,phase);w(0x10000-0x40,position);w(0x40000+0x3018,count);w(0x50000+0x7c,0x800a0000)
 u.mem_write(0x40000+0x50000-0x225c,bytes([mip])*3000)
 # Original layout begins its selected mip block with count unused frames.
 u.mem_write(0xa0000,bytes(count*(2048>>mip)*2)+struct.pack('<'+'h'*len(frames),*frames))
 u.emu_start(0x9d029f4c,0,count=1000)
 assert trace[-1]==0x9d02a08c,(hex(u.reg_read(UC_MIPS_REG_PC)),len(trace),list(map(hex,trace[-15:])),phase,inc,position,scale,mip,count)
 return struct.unpack('<I',u.mem_read(0x10000-0x50,4))[0],struct.unpack('<i',u.mem_read(0x10000-0x40,4))[0],ctypes.c_int32(u.reg_read(UC_MIPS_REG_V0)).value
rng=random.Random(260126);cases=0
for mip in range(11):
 for count in [2,4,8]:
  length=2048>>mip
  # Both full-scale opposite samples and noise expose signed arithmetic errors.
  frames=[rng.randrange(-32768,32768) for _ in range(count*length)];frames[:4]=[-32768,32767,32767,-32768]
  array=(ctypes.c_int16*len(frames))(*frames)
  for j in range(120):
   phase=rng.randrange(1<<32);inc=rng.randrange(1,100000001);scale=rng.choice([0,2048,4096,8192])
   pos=rng.choice([-1,-256,0,1,255,256,(count-1)*256-1,(count-1)*256,(count+1)*256,rng.randrange(count*256)])
   voice=Voice(phase,inc,pos,scale);expected=ref(phase,inc,pos,scale,mip,frames,count)
   result=lib.pw_core_tick(ctypes.byref(voice),array,count,mip)
   actual=(voice.phase,voice.position,result)
   assert actual==expected,(mip,count,phase,inc,pos,scale,actual,expected)
   cases+=1
print(f'PASS: {cases} bit-exact cases versus original MIPS oscillator slice; 11 mip sizes, 2/4/8 frames, phase wrap, pitch scale, signed samples and position boundaries.')
