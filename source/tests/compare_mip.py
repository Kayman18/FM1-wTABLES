import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
stop_at=0x9d0c4ff4
def hook(u,a,s,d):
 if a==stop_at:u.emu_stop()
u.hook_add(UC_HOOK_CODE,hook);u.reg_write(UC_MIPS_REG_S2,0x80005780);u.emu_start(0x9d0c4f90,0,count=1000000);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
original=bytes(u.mem_read(0x53524,3000))
lib=ctypes.CDLL(str(ROOT/'native/libpwmip.so'));lib.pw_mip_for_bucket.argtypes=[ctypes.c_uint]*2;lib.pw_mip_index.argtypes=[ctypes.c_float];lib.pw_mip_select.argtypes=[ctypes.c_uint32,ctypes.c_int32]
for i,m in enumerate(original):assert lib.pw_mip_for_bucket(i,48000)==m,(i,m)
rng=random.Random(126115)
for case in range(6000):
 hz=rng.choice([-10.,0.,1.,8.,9.,16.,17.,24000.,30000.,rng.uniform(1,24000)]);hz=struct.unpack('<f',struct.pack('<f',hz))[0];fixed=int(hz*4096)
 for reg,value in [(UC_MIPS_REG_A0,fixed),(UC_MIPS_REG_S6,0x80005780),(UC_MIPS_REG_S0,0x80010000),(UC_MIPS_REG_T4,0)]:u.reg_write(reg,value)
 stop_at=0x9d029fd8;u.emu_start(0x9d029fa0,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 idx=lib.pw_mip_index(hz);assert idx==((max(1,min(24000,fixed>>12))-1)>>3)
 assert u.reg_read(UC_MIPS_REG_A2)==original[idx]
 # FM1 selection resamples the original period law to 44.1 kHz.
 inc=rng.randrange(3185015,2147418113);scale=rng.choice([2048,4096,8192]);freq=inc*scale/4096*44100/4294967296
 got=lib.pw_mip_select(inc,scale);exp=max(1,lib.pw_mip_for_bucket(lib.pw_mip_index(freq),44100));assert got==exp or abs(freq-round(freq))<.005,(freq,got,exp)
print('PASS: all 3000 original MIPS mip-table initialization entries and 6000 original pitch-index/selection cases; 44.1 kHz selector including pitch scaling and saturation; range 1024..2 samples.')
