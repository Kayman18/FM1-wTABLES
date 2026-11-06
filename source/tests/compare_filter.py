"""Compare one filter step with the actual original MIPS instructions.
The unrelated parallel lanes run too; no filter arithmetic is replaced.
"""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
def stop(u,a,s,d):
 if a in [0x9d02a288,0x9d15e5ec]:u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
for r,v in [(UC_MIPS_REG_S1,0x80010000),(UC_MIPS_REG_FP,0x80020000),(UC_MIPS_REG_S2,0),(UC_MIPS_REG_S6,0x80030000),(UC_MIPS_REG_S7,0x80064d34)]:u.reg_write(r,v)
class State(ctypes.Structure):_fields_=[('low',ctypes.c_float),('band',ctypes.c_float),('high',ctypes.c_float)]
lib=ctypes.CDLL(str(ROOT/'native/libpwfilter.so'))
lib.pw_filter_tick.argtypes=[ctypes.POINTER(State)]+[ctypes.c_float]*3
lib.pw_filter_q.argtypes=[ctypes.c_uint];lib.pw_filter_q.restype=ctypes.c_float
lib.pw_filter_damping.argtypes=[ctypes.c_float]*2;lib.pw_filter_damping.restype=ctypes.c_float
# Execute source reciprocal resonance table initialization, retaining fused math.
u.emu_start(0x9d15e590,0,count=2000)
for i in range(101):
 q=struct.unpack('<f',u.mem_read(0x64f38+i*4,4))[0]
 assert abs(q-lib.pw_filter_q(i))<3e-7,(i,q,lib.pw_filter_q(i))
rng=random.Random(126105);worst=0
for i in range(6000):
 low=rng.uniform(-1,1);band=rng.uniform(-1,1);dry=rng.uniform(-1,1);g=rng.uniform(.001,1.414121);d=lib.pw_filter_damping(g,lib.pw_filter_q(rng.randrange(101)))
 # s1+0x100 is an independent source lane: g,d,input,low,band,high.
 u.mem_write(0x10120,struct.pack('<6f',g,d,dry,low,band,0))
 u.emu_start(0x9d02a1e4,0,count=100)
 assert u.reg_read(UC_MIPS_REG_PC)==0x9d02a288
 expected=struct.unpack('<3f',u.mem_read(0x1012c,12))
 state=State(low,band,0);lib.pw_filter_tick(ctypes.byref(state),dry,g,d)
 actual=(state.low,state.band,state.high)
 err=max(abs(a-b) for a,b in zip(actual,expected));worst=max(worst,err)
 assert err<0.000003,(i,actual,expected)
print(f'PASS: 6000 filter-state cases versus original MIPS recurrence; max absolute state error {worst:.4g}. All 101 resonance-table entries verified against original initialization.')
