"""Execute original sine/rate initializers and the source phase/lookup slice."""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
u.reg_write(UC_MIPS_REG_SP,0x80050000);u.reg_write(UC_MIPS_REG_S6,0x80005780)
stop_at=0x9d15e64c
def stop(u,a,s,d):
 if a==stop_at:u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
u.emu_start(0x9d15e5ec,0,count=2000000);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
hz=struct.unpack('<101f',u.mem_read(0x650cc,404));stop_at=0x9d15e690
u.emu_start(0x9d15e64c,0,count=2000000);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
sine=struct.unpack('<1024f',u.mem_read(0x653f4,4096))
class State(ctypes.Structure):_fields_=[('phase',ctypes.c_uint32)]
lib=ctypes.CDLL(str(ROOT/'native/libpwlfo.so'))
lib.pw_lfo_value.argtypes=[ctypes.POINTER(State)];lib.pw_lfo_value.restype=ctypes.c_float
lib.pw_lfo_tick.argtypes=[ctypes.POINTER(State),ctypes.c_int32];lib.pw_lfo_tick.restype=ctypes.c_float
lib.pw_lfo_hz.argtypes=[ctypes.c_int];lib.pw_lfo_hz.restype=ctypes.c_float
lib.pw_lfo_increment.argtypes=[ctypes.c_int];lib.pw_lfo_increment.restype=ctypes.c_int32
for i,x in enumerate(sine):assert lib.pw_lfo_value(ctypes.byref(State(i<<22)))==x
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
for i,x in enumerate(hz):
 assert lib.pw_lfo_hz(i)==x
 inc=int(f32(x*f32(4294967296.0/44100.0)));assert lib.pw_lfo_increment(i)==inc and lib.pw_lfo_increment(-i)==-inc
stop_at=0x9d02b5dc;rng=random.Random(126108)
for case in range(5000):
 phase=rng.randrange(1<<32);inc=rng.choice([0,1,-1,973915,-973915,0x7fffffff,-0x7fffffff,rng.randrange(-1000000,1000001)])
 for r,v in [(UC_MIPS_REG_T9,phase),(UC_MIPS_REG_T5,inc&0xffffffff),(UC_MIPS_REG_A2,0x17f1c),(UC_MIPS_REG_A0,0x80010000),(UC_MIPS_REG_S6,0x80005780)]:u.reg_write(r,v)
 u.emu_start(0x9d02b590,0,count=50);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 ref_phase=struct.unpack('<I',u.mem_read(0x10034,4))[0]
 ref_value=struct.unpack('<f',struct.pack('<I',u.reg_read(UC_MIPS_REG_F0)))[0]
 state=State(phase);actual=lib.pw_lfo_tick(ctypes.byref(state),inc)
 assert state.phase==ref_phase and actual==ref_value,(case,state.phase,ref_phase,actual,ref_value)
print('PASS: all 1024 sine entries and 101 rates match execution of original initializers; 5000 phase-wrap, stop, reverse and lookup cases match original MIPS slice exactly. 44.1 kHz increments verified.')
