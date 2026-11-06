"""Execute the original coefficient helper including original logf/expf.
Compare the translated coefficient path; the audio path uses stable delta
recurrence and compensated summation instead of subtracting nearly equal floats.
"""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000)
u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
def stop(u,a,s,d):
 if a in (0x9d000000,0x9d15e4a8):u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
f=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
# Validate sign/order of the MIPS MSUB instruction in the source shape table.
u.emu_start(0x9d15e478,0,count=100)
for index in [0,64,127]:
 u.reg_write(UC_MIPS_REG_S1,index);u.emu_start(0x9d15e498,0,count=20)
 actual=struct.unpack('<f',struct.pack('<I',u.reg_read(UC_MIPS_REG_F12)))[0]
 assert abs(actual-(index*0.03779527544975281-5))<5e-7
lib=ctypes.CDLL(str(ROOT/'native/libpwenv.so'))
lib.pw_env_coeff.argtypes=[ctypes.c_float]*4+[ctypes.POINTER(ctypes.c_float)]*2
lib.pw_env_ratio.argtypes=[ctypes.c_uint];lib.pw_env_ratio.restype=ctypes.c_float
rng=random.Random(1261);worst_c=worst_b=0
for case in range(400):
 current=rng.uniform(-1,1);target=rng.choice([-1.,0.,1.,rng.uniform(-1,1)]);inv=1/rng.choice([44.1,441,4410,44100,661500,1323000]);ratio=lib.pw_env_ratio(rng.randrange(128))
 u.mem_write(0x10000,struct.pack('<4f',current,0,0,0))
 for r,v in [(UC_MIPS_REG_A0,0x80010000),(UC_MIPS_REG_A1,f(inv)),(UC_MIPS_REG_A2,f(target)),(UC_MIPS_REG_A3,f(ratio)),(UC_MIPS_REG_SP,0x80050000),(UC_MIPS_REG_RA,0x9d000000)]:u.reg_write(r,v)
 u.emu_start(0x9d0e8058,0,count=100000)
 assert u.reg_read(UC_MIPS_REG_PC)==0x9d000000
 _,b,c,_=struct.unpack('<4f',u.mem_read(0x10000,16))
 cc=ctypes.c_float();bb=ctypes.c_float();lib.pw_env_coeff(current,target,inv,ratio,ctypes.byref(cc),ctypes.byref(bb))
 worst_c=max(worst_c,abs(c-cc.value));worst_b=max(worst_b,abs(b-bb.value))
 assert abs(c-cc.value)<3e-7 and abs(b-bb.value)<3e-7,(case,c,cc.value,b,bb.value)
print(f'PASS: 400 bipolar coefficient cases versus original MIPS helper and original math routines. Max coefficient error {worst_c:.3g}; offset error {worst_b:.3g}. Shape-table MSUB sign verified by execution.')
# Execute all 128 original shape-table entries, including original expf.
worst_shape=0
for index in range(128):
 u.reg_write(UC_MIPS_REG_S1,index);u.emu_start(0x9d15e498,0,count=20)
 u.reg_write(UC_MIPS_REG_SP,0x80050000);u.reg_write(UC_MIPS_REG_RA,0x9d000000)
 u.emu_start(0x9d0d264c,0,count=100000)
 assert u.reg_read(UC_MIPS_REG_PC)==0x9d000000
 actual=struct.unpack('<f',struct.pack('<I',u.reg_read(UC_MIPS_REG_F0)))[0]
 expected=lib.pw_env_ratio(index);err=abs(actual-expected);worst_shape=max(worst_shape,err)
 assert err<3e-7,(index,actual,expected)
print(f'PASS: all 128 shape ratios against original MIPS initializer/MSUB and expf, max error {worst_shape:.3g}.')
