"""Compare source filter routing arithmetic and source parameter ranges."""
import sys,struct,ctypes,random
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==0x9d029cdc else None)
L=ctypes.CDLL(str(R/'native/libpwsigned.so'));L.pw_filter_mod_signed.argtypes=[ctypes.c_float]*8;L.pw_filter_mod_signed.restype=ctypes.c_float
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def write(a,x):u.mem_write(a&0x1fffffff,struct.pack('<f',x))
def regfloat(reg,x):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',x))[0])
for index,limits in [(32,(-127,127)),(34,(-127,127)),(36,(-100,100)),(37,(-127,127))]:
 row=struct.unpack('<HhhhI',read(0x9d0889e4+index*16,12));assert row[1:3]==limits
rng=random.Random(126121);worst=0
for i in range(10000):
 base=f32(rng.uniform(0,127));track=f32(rng.uniform(-100,100));d1=rng.randrange(-127,128);d2=rng.randrange(-127,128);dl=rng.randrange(-127,128);a=f32(rng.random());b=f32(rng.random());lfo=f32(rng.uniform(-1,1))
 for reg,value in [(UC_MIPS_REG_S4,0x80020000),(UC_MIPS_REG_S1,0x80010000),(UC_MIPS_REG_A0,0x80030000),(UC_MIPS_REG_V0,0x80040000)]:u.reg_write(reg,value)
 regfloat(UC_MIPS_REG_F0,base);regfloat(UC_MIPS_REG_F5,d1)
 write(0x40000-0x265c,track);write(0x30004,a);write(0x20000-0x2264,d2);write(0x40000-0x266c,b);write(0x20000-0x2260,dl);write(0x40000-0x2680,lfo)
 u.emu_start(0x9d029c60,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==0x9d029cdc
 expected=struct.unpack('<f',struct.pack('<I',u.reg_read(UC_MIPS_REG_F3)))[0];expected=max(0,min(127,expected))
 actual=L.pw_filter_mod_signed(base,track,d1,a,d2,b,dl,lfo);err=abs(actual-expected);worst=max(worst,err);assert err<2e-5,(i,expected,actual)
print(f'PASS: 10000 original MIPS signed Amp/Env2/LFO cutoff sums with tracking, cancellation and final FM1 clamp; worst error {worst:.3g}. Original signed parameter ranges confirmed.')
