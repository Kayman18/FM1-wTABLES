"""Execute original decay completion including release coefficient calculation."""
import sys,struct,random,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read,words,text
from unicorn import *
from unicorn.mips_const import *
assert [text(w) for w in words(0x9d088ec4,3)]==['Gated','Drone','Triggered']
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop=0
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==stop else None)
def fw(a,v):u.mem_write(a&0x1fffffff,struct.pack('<f',v))
def fr(r,v):u.reg_write(r,struct.unpack('<I',struct.pack('<f',v))[0])
rng=random.Random(24)
for second in [False,True]:
 for mode in [0,1,2]:
  for case in range(128):
   level=rng.uniform(-1 if second else 0,1);release=case;shape=127-case
   inverse=1/(10*math.exp(release*0.06304226815700531)*48);ratio=math.exp(shape*0.03779527544975281-5)
   for reg,v in [(UC_MIPS_REG_S0,0x80010010),(UC_MIPS_REG_S6,0x80005780),(UC_MIPS_REG_S7,0x80075780),(UC_MIPS_REG_SP,0x800f0000),(UC_MIPS_REG_T0,0x9d0e0000)]:u.reg_write(reg,v)
   u.mem_write(0x6ee0e,struct.pack('<h',mode))
   u.mem_write(0x6ed78+(20 if second else 14)*2,struct.pack('<h',release));u.mem_write(0x6ed78+(22 if second else 16)*2,struct.pack('<h',shape))
   fw(0x5780+4*(0x17bac+release)+4,inverse);fw(0x5780+4*(0x17c2c+shape)+4,ratio)
   at=0x10010 if second else 0x10000;fw(at,level);fr(UC_MIPS_REG_F0,level);fr(UC_MIPS_REG_F24,1)
   stop=0x9d02a11c if second else 0x9d02a0fc
   u.emu_start(0x9d02a938 if second else 0x9d02aa08,0,count=10000)
   assert u.reg_read(UC_MIPS_REG_PC)==stop
   stage=u.mem_read(0x10029 if second else 0x10028,1)[0]
   assert stage==(4 if mode==2 else 1),(second,mode,stage)
   if mode==2:
    current,offset,coef=struct.unpack('<fff',u.mem_read(at,12))
    assert abs(current-level)<1e-6
    expected=math.exp(-inverse*math.log((abs(level)+ratio)/ratio))
    assert abs(coef-expected)<2e-7,(coef,expected)
print('PASS: triggered original decay completion: 768 cases across both envelopes, all three source modes, signed Env2, all release times/shapes; source Triggered transitions to release and other modes to sustain.')
for mode in [0,1,2]:
 u.mem_write(0x6ee0e,struct.pack('<h',mode));u.reg_write(UC_MIPS_REG_S7,0x80075780);u.reg_write(UC_MIPS_REG_T0,2);u.reg_write(UC_MIPS_REG_S3,0)
 stop=0x9d02b2cc if mode==2 else 0x9d0d79b8
 u.emu_start(0x9d02b2b0,0,count=20);assert u.reg_read(UC_MIPS_REG_PC)==stop
print('PASS: original Triggered gate-release branch skips note-off envelope release; Gated/Drone call the release helper.')
