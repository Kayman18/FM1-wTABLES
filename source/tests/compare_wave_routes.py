"""Compare FM1 wave mixing with the original isolated modulation lane.
Source normalization/operation order differs; allow one Q8 position unit.
The recovered oscillator's existing final position clamp is applied afterward.
"""
import sys,struct,random,ctypes
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop_at=0x9d029c38
def stop(u,a,s,d):
 if a==stop_at:u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
class Dummy:pass
lib=ctypes.CDLL(str(ROOT/'native/libpwwavemod.so'));lib.pw_wave_mod_position.argtypes=[ctypes.c_int32]+[ctypes.c_float]*6;lib.pw_wave_mod_position.restype=ctypes.c_int32
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def write(a,x):u.mem_write(a&0x1fffffff,struct.pack('<f',x))
def regfloat(reg,x):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',x))[0])
rng=random.Random(126109);worst=0
for case in range(6000):
 base=rng.randrange(1792);d1=rng.randrange(-100,101);d2=rng.randrange(-100,101);dl=rng.randrange(-100,101);e1=f32(rng.random());e2=f32(rng.random());lfo=f32(rng.uniform(-1,1))
 for r,v in [(UC_MIPS_REG_S1,0x80010000),(UC_MIPS_REG_S4,0x80020000),(UC_MIPS_REG_A0,0x80030000),(UC_MIPS_REG_V0,0x80040000),(UC_MIPS_REG_SP,0x80050000)]:u.reg_write(r,v)
 regfloat(UC_MIPS_REG_F4,base/1791.0);regfloat(UC_MIPS_REG_F6,e1)
 write(0x20000-0x227c,f32(d1*f32(.01)));write(0x20000-0x2278,f32(d2*f32(.01)));write(0x20000-0x2274,f32(dl*f32(.01)));write(0x20000-0x2298,1791)
 write(0x10000+0xd0,e2);write(0x10000+0xbc,lfo)
 u.emu_start(0x9d029b80,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 expected=struct.unpack('<i',u.mem_read(0x10090,4))[0];expected=max(0,min(1791,expected))
 actual=lib.pw_wave_mod_position(base,d1,e1,d2,e2,dl,lfo);error=abs(actual-expected);worst=max(worst,error)
 assert error<=1,(case,actual,expected,base,d1,d2,dl,e1,e2,lfo)
print(f'PASS: 6000 signed Amp/Env2/LFO wave mixtures versus the original MIPS lane, including final position clamp. Maximum difference {worst} Q8 unit (normalization/float operation order).')
