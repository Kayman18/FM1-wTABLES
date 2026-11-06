"""Execute original phase spread and Poly/Mono/Off note-on branches."""
import sys,struct,random,ctypes
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
def stop(u,a,s,d):
 if a in (0x9d09d0bc,0x9d09d0c0):u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
lib=ctypes.CDLL(str(ROOT/'native/libpwlfophase.so'))
lib.pw_lfo_assign_phase.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_int,ctypes.c_int]
rng=random.Random(126111)
for case in range(8000):
 count=rng.randrange(1,9);slot=rng.randrange(count);mode=rng.randrange(3);spread=rng.randrange(91)
 phases=[rng.randrange(1<<32) for _ in range(count)]
 stages=[rng.choice([0,0,4,4]) for _ in range(count)] if case%2 else [rng.randrange(5) for _ in range(count)]
 any_held=any(1<=s<=3 for s in stages)
 for i,(phase,stage) in enumerate(zip(phases,stages)):
  u.mem_write(0x530fc+i*128,struct.pack('<I',phase));u.mem_write(0x5312c+i*128,bytes([stage]))
 for r,v in [(UC_MIPS_REG_T1,slot*spread),(UC_MIPS_REG_A1,spread),(UC_MIPS_REG_A3,mode),(UC_MIPS_REG_T0,1),(UC_MIPS_REG_T3,count),(UC_MIPS_REG_T5,0x80055780),(UC_MIPS_REG_A2,0x80055780+slot*128),(UC_MIPS_REG_S0,0x80005780),(UC_MIPS_REG_S3,slot*128)]:u.reg_write(r,v)
 u.emu_start(0x9d09d080,0,count=500)
 assert u.reg_read(UC_MIPS_REG_PC) in (0x9d09d0bc,0x9d09d0c0)
 expected=[struct.unpack('<I',u.mem_read(0x530fc+i*128,4))[0] for i in range(count)]
 actual=(ctypes.c_uint32*count)(*phases);lib.pw_lfo_assign_phase(actual,count,slot,mode+1,spread,any_held,1)
 assert list(actual)==expected,(case,count,slot,mode,spread,stages,phases,list(actual),expected)
print('PASS: 8000 original MIPS phase-spread and Poly/Mono/Off note-on cases match exactly, including 1–8 slots, held/release-only groups, shared-anchor continuation and 32-bit phase wrap.')
