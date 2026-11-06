import sys,ctypes,struct,random
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
loads={0x9d02a52c:(UC_MIPS_REG_F0,UC_MIPS_REG_A0),0x9d02a530:(UC_MIPS_REG_F2,UC_MIPS_REG_A2),0x9d02a534:(UC_MIPS_REG_F3,UC_MIPS_REG_T0),0x9d02a538:(UC_MIPS_REG_F1,UC_MIPS_REG_A3)}
stores={0x9d02a56c:UC_MIPS_REG_F8,0x9d02a570:UC_MIPS_REG_F7,0x9d02a574:UC_MIPS_REG_F6,0x9d02a578:UC_MIPS_REG_F5}
stop_at=0;history=[];written=[]
def getf(reg):return struct.unpack('<f',struct.pack('<I',u.reg_read(reg)))[0]
def setf(reg,x):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',x))[0])
def hook(u,a,s,d):
 if a==stop_at:u.emu_stop();return
 if a in loads:
  reg,idx=loads[a];setf(reg,history[u.reg_read(idx)//4]);u.reg_write(UC_MIPS_REG_PC,a+4)
 if a in stores:written.append(getf(stores[a]));u.reg_write(UC_MIPS_REG_PC,a+4)
u.hook_add(UC_HOOK_CODE,hook)
L=ctypes.CDLL(str(R/'native/libpwdelay.so'));P=ctypes.POINTER(ctypes.c_float);L.pw_delay_frame.argtypes=[P,P,P,P,ctypes.c_uint,ctypes.c_float,ctypes.c_float]
rng=random.Random(126119);worst=0
for case in range(6000):
 mode=case%2+1;fb=rng.randrange(101)*.01;level=rng.random();input=[rng.uniform(-1,1) for _ in range(4)];history=[rng.uniform(-1,1) for _ in range(4)];written=[]
 w=rng.randrange(1<<19);samples=rng.uniform(48,480000)
 for addr,value,fmt in [(0x56f44,w,'<I'),(0x56f54,samples,'<f'),(0x56f4c,fb,'<f'),(0x56f50,level,'<f'),(0x6edea,mode,'<H')]:u.mem_write(addr,struct.pack(fmt,value))
 for reg,value in [(UC_MIPS_REG_S4,0x80055780),(UC_MIPS_REG_S7,0x80075780),(UC_MIPS_REG_T1,3)]:u.reg_write(reg,value)
 for reg,x in zip([UC_MIPS_REG_F11,UC_MIPS_REG_F10,UC_MIPS_REG_F12,UC_MIPS_REG_F9],input):setf(reg,x)
 stop_at=0x9d02b6a0;u.emu_start(0x9d02a4dc,0,count=150);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 expected=[getf(reg) for reg in [UC_MIPS_REG_F0,UC_MIPS_REG_F2,UC_MIPS_REG_F3,UC_MIPS_REG_F4]]
 a=(ctypes.c_float*4)(*input);h=(ctypes.c_float*4)(*history);wr=(ctypes.c_float*4)();out=(ctypes.c_float*4)();L.pw_delay_frame(a,h,wr,out,mode,fb,level)
 err=max(max(abs(a-b) for a,b in zip(expected,out)),max(abs(a-b) for a,b in zip(written,wr)));worst=max(worst,err);assert err<8e-7,(case,expected,list(out),written,list(wr))
 assert struct.unpack('<I',u.mem_read(0x56f44,4))[0]==(w+1)&0x7ffff
print(f'PASS: 6000 original MIPS Stereo/Ping-pong four-lane feedback, return and write-index cases; max error {worst:.3g}. Indexed reads/writes mapped to supplied frames, no arithmetic replaced.')
