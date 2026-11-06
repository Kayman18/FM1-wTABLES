"""Execute original Env2/LFO pitch mixing and exponent-table conversion.
Relative Q12 normalization divides the source increment by 110*64.
"""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
assert struct.unpack("<HhhhI",read(0x9d0889e4+19*16,12))[1:3]==(-127,127)
assert struct.unpack("<HhhhI",read(0x9d0889e4+38*16,12))[1:3]==(-120,120)
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop_at=0x9d029f2c
indexed_loads=0
def hook(u,a,s,d):
 global indexed_loads
 if a==stop_at:u.emu_stop();return
 # Unicorn's PIC32 indexed LWX support varies. Implement the table load
 # exactly; every subsequent source shift/add/selection executes normally.
 if a==0x9d02b57c:
  address=(u.reg_read(UC_MIPS_REG_A3)+u.reg_read(UC_MIPS_REG_V1))&0x1fffffff
  u.reg_write(UC_MIPS_REG_T7,struct.unpack('<I',u.mem_read(address,4))[0]);u.reg_write(UC_MIPS_REG_PC,a+4);indexed_loads+=1
u.hook_add(UC_HOOK_CODE,hook)
lib=ctypes.CDLL(str(ROOT/'native/libpwenvpitch.so'))
lib.pw_env_pitch_units.argtypes=[ctypes.c_float]*3;lib.pw_env_pitch_units.restype=ctypes.c_int32
lib.pw_env_pitch_scale.argtypes=[ctypes.c_int32];lib.pw_env_pitch_scale.restype=ctypes.c_int32
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def fw(a,v):u.mem_write(a&0x1fffffff,struct.pack('<f',v))
def fr(reg,v):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',v))[0])
rng=random.Random(126112);worst=0
for case in range(8000):
 env=f32(rng.uniform(-1,1));depth=rng.randrange(-120,121);lfo=f32(rng.uniform(-1,1));lfodepth=rng.randrange(-1200,1201)
 for r,v in [(UC_MIPS_REG_S4,0x80010000),(UC_MIPS_REG_V0,0x80020000),(UC_MIPS_REG_S1,0x80030000)]:u.reg_write(r,v)
 fw(0x10000-0x2284,f32(depth*f32(4369.06689453125)));fw(0x10000-0x2280,f32(f32(lfodepth/10)*f32(4369.06689453125)))
 fr(UC_MIPS_REG_F10,env);fr(UC_MIPS_REG_F14,lfo);stop_at=0x9d029f2c
 u.emu_start(0x9d029ebc,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 expected_units=struct.unpack('<i',u.mem_read(0x300f4,4))[0]
 actual_units=lib.pw_env_pitch_units(env,depth,f32(lfo*lfodepth));err=abs(expected_units-actual_units);worst=max(worst,err);assert err<=1,(case,actual_units,expected_units)
 # Exercise exponent boundaries plus random relative offsets over +/-2 oct.
 units=rng.choice([-1048576,-524289,-524288,-1,0,1,524287,524288,1048576,rng.randrange(-1048576,1048577)])
 for r,v in [(UC_MIPS_REG_V0,(6<<19)+units),(UC_MIPS_REG_A3,0x9d02c72c),(UC_MIPS_REG_A0,0x80040000),(UC_MIPS_REG_T0,6),(UC_MIPS_REG_T1,0),(UC_MIPS_REG_T5,0),(UC_MIPS_REG_A2,0x17f1c)]:u.reg_write(r,v)
 stop_at=0x9d02b5d8;u.emu_start(0x9d02b574,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 increment=struct.unpack('<I',u.mem_read(0x40000,4))[0]
 assert lib.pw_env_pitch_scale(units)==increment//7040,(case,units,increment,lib.pw_env_pitch_scale(units))
print(f'PASS: 8000 original MIPS Env2/LFO pitch mixtures (max {worst} Q19 unit difference) and exact relative-Q12 exponent conversions, including negative/wrap-boundary fractions. {indexed_loads} indexed table loads emulated; no pitch arithmetic replaced.')
