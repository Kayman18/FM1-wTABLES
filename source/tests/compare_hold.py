"""Execute original global sustain OR/pedal and held-note release selection."""
from pathlib import Path
import sys,struct
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from inspect_firmware import read,words,text
from unicorn import *
from unicorn.mips_const import *
w=words(0x9d0889e4+41*16,4)
assert text(w[2])=='Sustain' and (w[0]>>16)==0 and (w[1]&65535)==1
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000)
u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
released=[]
def hook(u,a,size,data):
 if a==0x9d1fff00:u.emu_stop()
 elif a==0x9d0d79b8:
  released.append(u.reg_read(UC_MIPS_REG_A0))
  u.reg_write(UC_MIPS_REG_PC,u.reg_read(UC_MIPS_REG_RA))
u.hook_add(UC_HOOK_CODE,hook)
for hold in [0,1]:
 for pedal in [0,1]:
  for mask in range(256):
   u.mem_write(0x50000,bytes(0x10000))
   u.mem_write(0x6edca,struct.pack('<h',hold));u.mem_write(0x5411c,bytes([pedal]))
   u.mem_write(0x54124,struct.pack('<I',mask<<16))
   u.mem_write(0x54130,b'\xff'*16);u.mem_write(0x54141,b'\x08')
   for slot in range(8):u.mem_write(0x53134+128*slot,bytes([48+slot]))
   u.reg_write(UC_MIPS_REG_SP,0x800f0000);u.reg_write(UC_MIPS_REG_RA,0x9d1fff00)
   u.mem_write(0xf0024,struct.pack('<I',0x9d1fff00));released.clear()
   u.emu_start(0x9d0e3b38,0,count=2000)
   assert u.reg_read(UC_MIPS_REG_PC)==0x9d1fff00
   assert u.mem_read(0x56edb,1)[0]==bool(hold or pedal)
   assert released==([] if hold or pedal else [i for i in range(8) if not(mask>>i&1)]),(hold,pedal,mask,released)
   if not(hold or pedal):assert u.mem_read(0x54130,16)==bytes(16) and u.mem_read(0x54141,1)==b'\x00'
print('PASS: original global Sustain: 1024 executed cases, Hold OR pedal, all 256 held-note masks, released-slot selection and sustained-bookkeeping clear. External release helper calls recorded, not emulated.')
