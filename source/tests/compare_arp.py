"""Full original deterministic arpeggio routine, including source sorting."""
import sys,struct,ctypes,random,re
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read,words,text,md
from unicorn import *
import unicorn.mips_const as M
assert [text(x) for x in words(0x9d0827e4,5)]==['Off','Up','Down','Alt','Alt2']
assert [text(x) for i,x in enumerate(words(0x9d0827e4,15)) if i in [9,10,14]]==['Step Up','Step Down','As Played']
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(M.UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
loads={}
for a,n in [(0x9d07f804,0x6f4),(0x9d0f9d90,52),(0x9d0f0708,256),(0x9d0ef81c,144)]:
 for i in md.disasm(read(a,n),a):
  if i.mnemonic in ['lwx','lbux']:
   dst,idx,base=re.fullmatch(r'\$(\w+), \$(\w+)\(\$(\w+)\)',i.op_str).groups()
   loads[i.address]=(getattr(M,'UC_MIPS_REG_'+dst.upper()),getattr(M,'UC_MIPS_REG_'+idx.upper()),getattr(M,'UC_MIPS_REG_'+base.upper()),4 if i.mnemonic=='lwx' else 1)
def hook(u,a,size,data):
 if a==0x9d1fff00:u.emu_stop()
 elif a in loads:
  d,i,b,n=loads[a];addr=(u.reg_read(i)+u.reg_read(b))&0x1fffffff
  u.reg_write(d,int.from_bytes(u.mem_read(addr,n),'little'));u.reg_write(M.UC_MIPS_REG_PC,a+4)
u.hook_add(UC_HOOK_CODE,hook)
lib=ctypes.CDLL(str(R/'native/libarp.so'));lib.pw_arp_next.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
rng=random.Random(30);cases=0
for count in [1,2,3,4,5,8,16]:
 for mode in range(1,8):
  for octaves in range(1,4):
   for variant in range(3):
    notes=[24+rng.randrange(60) for _ in range(count)]
    if variant==1:notes.sort(reverse=True)
    if variant==2:notes=[48]*count
    u.mem_write(0x20000,bytes(notes));u.mem_write(0x30000,bytes(128));u.mem_write(0x30004,b'\xff')
    u.mem_write(0x40000,b'\x01\x00'+bytes(10));step=ctypes.c_uint8(255)
    for k in range(100):
     for reg,val in [('A0',0x80020000),('A1',count),('A2',0),('A3',0x80030000),('SP',0x800f0000),('RA',0x9d1fff00)]:u.reg_write(getattr(M,'UC_MIPS_REG_'+reg),val)
     u.mem_write(0xf0010,struct.pack('<III',{5:9,6:10,7:14}.get(mode,mode),octaves,0x80040000))
     u.emu_start(0x9d07f804,0,count=20000)
     assert u.reg_read(M.UC_MIPS_REG_PC)==0x9d1fff00
     index=lib.pw_arp_next(ctypes.byref(step),count,mode,octaves)
     base=(notes if mode==7 else sorted(notes))[index%count];expected=base+12*(index//count)
     if expected>=128:expected=base
     actual=u.reg_read(M.UC_MIPS_REG_V0)
     assert actual==expected,(count,mode,octaves,k,notes,actual,expected,index)
     assert u.mem_read(0x30004,1)[0]==step.value
     cases+=1
print(f'PASS: arpeggio source: {cases} exact original MIPS outputs and step states for Up/Down/Alt/Alt2/Step Up/Step Down/As Played, 1..3 octaves, unsorted and duplicate inputs; original sorting executed, indexed loads emulated only.')
