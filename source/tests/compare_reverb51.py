"""Verify production kernel at source rate against unmodified original MIPS DSP."""
import ctypes as C,struct,sys
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
class Ring(C.Structure):_fields_=[('v',C.POINTER(C.c_float)),('n',C.c_uint),('w',C.c_uint)]
class S(C.Structure):
 _fields_=[('early',Ring),('align',Ring),('comb',Ring*6),('ap',Ring*2),('taps',C.c_uint16*6),('delays',C.c_uint16*6),('apdelay',C.c_uint16*2),('align_delay',C.c_uint16)]+[(x,C.c_float*6) for x in ['gains','low','damp','feedback','target_damp','target_feedback']]+[(x,C.c_float) for x in ['depth','target_depth','rate_ratio']]+[(x,C.c_uint32) for x in ['phase','step','used','clear_left','smooth','model','size','time','damping','speed','moddepth']]
lib=C.CDLL(str(R/'native/libreverb51.so'));lib.pw_reverb_configure.argtypes=[C.POINTER(S),C.POINTER(C.c_float),C.c_uint,C.c_uint,C.c_uint,C.c_uint];lib.pw_reverb_tick.argtypes=[C.POINTER(S),C.c_float,C.POINTER(C.c_float)]
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
def stop(u,a,n,d):
 if a==0x9d0369f4:u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
def wr(a,fmt,*v):u.mem_write(a,struct.pack(fmt,*v))
def fp(reg,v):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',v))[0])
rng=np.random.default_rng(151);worst=0;worst_state=0;base=0x5780
for case in range(500):
 s=S();mem=(C.c_float*67584)();lib.pw_reverb_configure(C.byref(s),mem,len(mem),case%4,int(rng.integers(1,101)),96000)
 s.clear_left=s.smooth=0;s.depth=int(rng.integers(0,101));s.phase=int(rng.integers(0,2**32));s.step=int(rng.integers(1,223697));s.low[:]=rng.uniform(-.2,.2,6);s.damp[:]=rng.uniform(0,.95,6);s.feedback[:]=rng.uniform(0,.7,6)
 original=[]
 for r in [s.early,s.align,*s.comb,*s.ap]:
  a=rng.uniform(-.5,.5,r.n).astype('<f4');C.memmove(r.v,a.ctypes.data,a.nbytes)
  n=8192 if len(original)<8 else 1024;b=np.zeros(n,dtype='<f4');d=np.arange(r.n);b[(-d)%n]=a[(-d)%r.n];original.append(b)
 for i,a in enumerate(original[:8]):u.mem_write(base+0x3dc+i*32768,a.tobytes())
 u.mem_write(base+0x403dc,np.stack(original[8:],axis=1).astype('<f4').tobytes())
 # Source callback supplies raw early taps; native has already applied the mask.
 wr(base+0x34e,'<6H',*s.delays);wr(base+0x35a,'<6H',*s.taps)
 # Keep source alignment offset equal to the configured native offset, including
 # Hall 2's wrapped early taps: raw last tap controls alignment independently.
 raw_last=int(s.taps[5]);
 if s.align_delay:raw_last=int(s.delays[0])+s.align_delay
 wr(base+0x364,'<H',raw_last)
 wr(base+0x374,'<6f',*s.gains);wr(base+0x38c,'<6f',*s.low);wr(base+0x3a4,'<6f',*s.damp);wr(base+0x3bc,'<6f',*s.feedback);wr(base+0x3d4,'<2f',1.,1.);wr(base+0x368,'<2I',s.phase,s.step);wr(0x6ede6,'<h',int(s.depth))
 for reg,val in [(UC_MIPS_REG_S2,0x80005780),(UC_MIPS_REG_S6,0x80075780),(UC_MIPS_REG_SP,0x800f0000),(UC_MIPS_REG_T5,0),(UC_MIPS_REG_T6,0),(UC_MIPS_REG_T4,0),(UC_MIPS_REG_T1,0)]:u.reg_write(reg,val)
 x=float(rng.uniform(-.5,.5));fp(UC_MIPS_REG_F22,x);fp(UC_MIPS_REG_F5,2**-20);fp(UC_MIPS_REG_F4,1/6);fp(UC_MIPS_REG_F19,.7);fp(UC_MIPS_REG_F18,.69);wr(0xf0010,'<I',0x9d0e0000)
 u.emu_start(0x9d0367c8,0,count=10000);assert u.reg_read(UC_MIPS_REG_PC)==0x9d0369f4
 out=(C.c_float*2)();lib.pw_reverb_tick(C.byref(s),x,out);expected=struct.unpack('<2f',u.mem_read(base+0x260,8));err=max(abs(a-b) for a,b in zip(out,expected));worst=max(worst,err);assert err<1.5e-5,(case,list(out),expected,err)
 state_error=float(np.max(np.abs(np.array(struct.unpack('<6f',u.mem_read(base+0x38c,24)))-s.low)));worst_state=max(worst_state,state_error);assert state_error<1.5e-5,(case,state_error)
 for i in range(6):assert abs(struct.unpack('<f',u.mem_read(base+0x103dc+i*32768,4))[0]-s.comb[i].v[0])<1.5e-5
print('PASS: production C core, 500 randomized original-MIPS comparisons at source rate; max output error',worst,'max state error',worst_state)
