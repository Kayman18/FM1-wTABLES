"""Execute original Env2/LFO pitch mixing and exponent-table conversion.
Relative Q12 normalization divides the source increment by 110*64.
"""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
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
lib=ctypes.CDLL(str(ROOT/'native/libpwperformance.so'))
lib.pw_performance_increment.argtypes=[ctypes.c_uint32,ctypes.c_int32];lib.pw_performance_increment.restype=ctypes.c_int32
lib.pw_performance_units.argtypes=[ctypes.c_float];lib.pw_performance_units.restype=ctypes.c_int32
lib.pw_performance_bend.argtypes=[ctypes.c_int,ctypes.c_uint];lib.pw_performance_bend.restype=ctypes.c_float
import math
rng=random.Random(126114);worst=0
for case in range(6000):
 units=rng.randrange(-5*524288,5*524288+1);base_inc=rng.randrange(3185015,96200120)
 for r,v in [(UC_MIPS_REG_V0,(6<<19)+units),(UC_MIPS_REG_A3,0x9d02c72c),(UC_MIPS_REG_A0,0x80040000),(UC_MIPS_REG_T0,6),(UC_MIPS_REG_T1,0),(UC_MIPS_REG_T5,0),(UC_MIPS_REG_A2,0x17f1c)]:u.reg_write(r,v)
 stop_at=0x9d02b5d8;u.emu_start(0x9d02b574,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 source=struct.unpack('<I',u.mem_read(0x40000,4))[0]
 actual=lib.pw_performance_increment(base_inc,units);expected=min(2147418112,base_inc*source//(110*262144))
 assert abs(actual-expected)<=4,(case,units,actual,expected)
 # Exact widened adaptation: same source mantissa and floor exponent.
 mant=struct.unpack('<I',read(0x9d02c72c+(((units>>9)&1023)*4),4))[0]
 assert actual==min(2147418112,(base_inc*mant)>>(18-(units//524288)))
for i in range(-86000,86001,17):
 cents=i*.1;units=lib.pw_performance_units(cents);base_inc=3185015;actual=lib.pw_performance_increment(base_inc,units)
 if actual==2147418112:continue
 err=abs(1200*math.log2(actual/base_inc)-cents);worst=max(worst,err);assert err<1.27,(cents,err)
for r in range(25):
 assert lib.pw_performance_bend(-8192,r)==-100*r
 assert abs(lib.pw_performance_bend(8191,r)-8191*r*100/8192)<.001
 assert lib.pw_performance_bend(0,r)==0
print(f'PASS: 6000 original MIPS exponent conversions within source-increment rounding, exact widened relative conversion, +/-8600-cent curve max error {worst:.4f} cents, all bend ranges/endpoints, Nyquist increment limiting.')

assert lib.pw_performance_increment(0xffffffff,5242880)==2147418112
assert lib.pw_performance_increment(3185015,-5242880)>0
