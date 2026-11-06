"""Original MIPS direction/countdown setup and subsequent member selection."""
import sys,struct,ctypes
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read,words,text
from unicorn import *
from unicorn.mips_const import *
assert [text(x) for x in words(0x9d082e4c,3)]==['Up','Down','Alternating']
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
stop=0
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==stop else None)
def wr(a,fmt,*v):u.mem_write(a&0x1fffffff,struct.pack(fmt,*v))
def regs():
 for reg,v in [(UC_MIPS_REG_S4,0x80055780),(UC_MIPS_REG_S6,0x80005780),(UC_MIPS_REG_S7,0x80075780),(UC_MIPS_REG_SP,0x800f0000)]:u.reg_write(reg,v)
lib=ctypes.CDLL(str(R/'native/libbreak.so'));count=0
for ms in [1,2,5,17,100,333,999,1000]:
 for mode in range(3):
  for previous in range(2):
   for voices in range(2,5):
    regs();wr(0x568ab,'BB',0,previous);wr(0x6ee10,'hh',ms,mode)
    for reg,v in [(UC_MIPS_REG_S2,voices),(UC_MIPS_REG_S5,3),(UC_MIPS_REG_S3,99),(UC_MIPS_REG_S0,7)]:u.reg_write(reg,v)
    stop=0x9d09caa0;u.emu_start(0x9d02b0bc,0,count=300)
    assert u.reg_read(UC_MIPS_REG_PC)==stop
    delay,origin,next_note,direction,vel,meta=struct.unpack('<IBBBBB',u.mem_read(0x568b0,9))
    assert delay==ms*48 and origin==3 and next_note==1 and vel==99 and meta==7
    assert direction==lib.pw_break_direction(mode,previous)
    assert u.reg_read(UC_MIPS_REG_A1)==(voices-1 if direction else 0)
    assert u.mem_read(0x568ac,1)[0]==direction
    # Execute the original per-sample decrement and next-member selection.
    wr(0x568b0,'I',1);wr(0x54144+77*3,'B',voices)
    regs();u.reg_write(UC_MIPS_REG_S3,0);u.reg_write(UC_MIPS_REG_S5,5<<16)
    u.emu_start(0x9d02b410,0,count=300)
    assert u.reg_read(UC_MIPS_REG_PC)==stop
    assert u.reg_read(UC_MIPS_REG_A1)==(voices-2 if direction else 1)
    assert int.from_bytes(u.mem_read(0x568b0,4),'little')==0
    # Reload interval reads the current source parameter, not the initial value.
    regs();u.reg_write(UC_MIPS_REG_FP,0x80055780);u.reg_write(UC_MIPS_REG_S1,1)
    wr(0x6ee10,'h',ms+1);stop=0x9d02b500
    u.emu_start(0x9d02b4dc,0,count=40)
    assert u.reg_read(UC_MIPS_REG_PC)==stop
    assert int.from_bytes(u.mem_read(0x568b0,4),'little')==(ms+1)*48
    count+=1
for ms in range(1001):
 for ordinal in range(4):assert lib.pw_break_delay(ms,ordinal)==(ms*441*ordinal+5)//10
print(f'PASS: break source: {count} original MIPS setup/first/next/reload cases, all directions and previous states, 2..4 notes; 4004 FM1 44.1kHz rounded offsets. No arithmetic hooks. FM1 snapshots spacing per strike, unlike live source reload.')
