"""Run the original Drone gate-threshold dispatch, no arithmetic hooks."""
import sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000)
u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==0x9d02acdc else None)
for mode in range(3):
 for threshold in (0,1,100,1000000):
  u.mem_write(0x6ee0e,struct.pack('<h',mode))
  u.reg_write(UC_MIPS_REG_S7,0x80075780);u.reg_write(UC_MIPS_REG_V0,threshold);u.reg_write(UC_MIPS_REG_A1,0)
  u.emu_start(0x9d02acc0,0,count=30)
  assert u.reg_read(UC_MIPS_REG_S1)==(0xff800001 if mode==1 else threshold)
  assert u.reg_read(UC_MIPS_REG_T9)==(0xffc00000 if mode==1 else threshold>>1)
print('PASS: 12 original MIPS gate-dispatch cases; Drone forces continuously active CV gate thresholds, not envelope bypass.')
