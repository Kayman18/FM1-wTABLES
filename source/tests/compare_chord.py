"""Compare native chord pitches with executed original MIPS, not a second formula."""
import ctypes,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
def hook(u,a,s,d):
 if a==0x9d000000:u.emu_stop()
 elif a==0x9d0f0f08: # PIC32 indexed word load only; all arithmetic executes unchanged.
  addr=(u.reg_read(UC_MIPS_REG_A0)+u.reg_read(UC_MIPS_REG_V0))&0x1fffffff
  u.reg_write(UC_MIPS_REG_A0,int.from_bytes(u.mem_read(addr,4),'little'));u.reg_write(UC_MIPS_REG_PC,a+4)
u.hook_add(UC_HOOK_CODE,hook)
lib=ctypes.CDLL(str(R/'native/libchord.so'));fn=lib.pw_chord_notes;fn.argtypes=[ctypes.c_uint,ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint8)]
assert [lib.pw_chord_shape(i) for i in range(15)]==[0,0,1,2,3,4,5,6,7,8,9,11,10,12,13]
out=(ctypes.c_uint8*5)();cases=0
for note in range(24,84):
 for scale in range(9):
  for shape in range(14):
   for inv in range(4):
    key=(cases%25)-12
    u.mem_write(0x10000,struct.pack('<5h',1,key,scale,shape,inv))
    for reg,v in [(UC_MIPS_REG_A0,note),(UC_MIPS_REG_A1,0x80011000),(UC_MIPS_REG_A2,0x80010000),(UC_MIPS_REG_A3,0x9d082d40+12*shape),(UC_MIPS_REG_SP,0x800f0000),(UC_MIPS_REG_RA,0x9d000000)]:u.reg_write(reg,v)
    u.emu_start(0x9d0d3c6c,0,count=3000);assert u.reg_read(UC_MIPS_REG_PC)==0x9d000000
    n=u.reg_read(UC_MIPS_REG_V0);expected=bytes(u.mem_read(0x11000,n));got=fn(note,key,scale,shape,inv,out)
    assert bytes(out[:got])==expected,(note,key,scale,shape,inv,list(expected),list(out[:got]))
    cases+=1
print(f'PASS: chord original MIPS exact: {cases} cases; all 60 FM1 input pitches, nine scales, 14 source shapes, four inversions; rotating all 25 keys. Only indexed load emulated.')
