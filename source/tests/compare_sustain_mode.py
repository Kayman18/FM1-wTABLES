"""Execute recovered Synth/Piano re-strike branch. Stub only external release calls."""
import struct,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read,words,text
from unicorn import *
from unicorn.mips_const import *
p=words(0x9d0889e4+66*16+12,1)[0];assert [text(x) for x in words(p,2)]==['Synth','Piano']
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));calls=[];end=0
def hook(u,a,size,data):
 if a==end:u.emu_stop()
 elif a==0x9d0d79b8:
  calls.append(u.reg_read(UC_MIPS_REG_A0));u.reg_write(UC_MIPS_REG_PC,u.reg_read(UC_MIPS_REG_RA))
u.hook_add(UC_HOOK_CODE,hook)
count=0
for mode in range(2):
 for note in [24,48,83]:
  for mask in range(256):
   calls.clear();end=0x9d09cc2c if mode==0 else 0x9d09cc6c
   for reg,v in [(UC_MIPS_REG_FP,0x80075780),(UC_MIPS_REG_V0,0x80055780),(UC_MIPS_REG_V1,3),(UC_MIPS_REG_S3,note),(UC_MIPS_REG_SP,0x800f0000)]:u.reg_write(reg,v)
   u.mem_write(0x6edfc,struct.pack('<h',mode))
   for slot in range(8):u.mem_write(0x53134+slot*128,bytes([note if mask&(1<<slot) else note+1]))
   u.emu_start(0x9d09cbd8,0,count=300)
   assert u.reg_read(UC_MIPS_REG_PC)==end
   assert calls==([slot for slot in range(8) if mask&(1<<slot)] if mode else [])
   assert u.mem_read(0x54141,1)==b'\x02'
   count+=1
print(f'PASS: source sustain responses: {count} executed-original Synth/Piano cases, all eight-slot matching masks and three pitches. Synth bypasses release/allocation; Piano releases matching voices then continues allocation. External release calls recorded, not executed.')
