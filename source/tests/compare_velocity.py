"""Run original MIPS arithmetic and note-velocity mapping; no arithmetic hooks."""
import sys,ctypes,struct
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000))
u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop_at=0
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==stop_at else None)
lib=ctypes.CDLL(str(R/'native/libpwvelocity.so'));lib.pw_velocity_gain.argtypes=[ctypes.c_uint,ctypes.c_float];lib.pw_velocity_gain.restype=ctypes.c_float
lib.pw_velocity_map.argtypes=[ctypes.c_uint]*3;lib.pw_velocity_map.restype=ctypes.c_uint
worst=0
for amount in range(101):
 for vel in range(128):
  slot=(amount+vel)%8;u.mem_write(0x6edac,struct.pack('<h',amount));u.mem_write(0x5312e+slot*128,bytes([vel]));u.reg_write(UC_MIPS_REG_A0,slot)
  stop_at=0x9d0ef1b4;u.emu_start(0x9d0ef12c,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
  expected=struct.unpack('<f',u.mem_read(0x530d0+slot*128,4))[0];actual=lib.pw_velocity_gain(vel,amount)
  error=abs(actual-expected);worst=max(worst,error);assert error<1e-7,(amount,vel,expected,actual)
stop_at=0x9d000000;u.reg_write(UC_MIPS_REG_RA,stop_at)
u.emu_start(0x9d168360,0,count=10000);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
for curve in range(4):
 for gate in range(2):
  for vel in range(128):
   u.mem_write(0x6edfe,struct.pack('<h',curve))
   for reg,val in [(UC_MIPS_REG_A0,2),(UC_MIPS_REG_A1,4),(UC_MIPS_REG_A2,vel),(UC_MIPS_REG_A3,gate),(UC_MIPS_REG_RA,0x9d000000)]:u.reg_write(reg,val)
   stop_at=0x9d000000;u.emu_start(0x9d0ee2e0,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
   expected=u.mem_read(0x56ed5,1)[0];assert lib.pw_velocity_map(vel,gate,curve)==expected
print(f'PASS: 12928 original MIPS velocity gain cases (amount0..100, velocity0..127), worst error {worst:.3g}; 1024 original Linear/Fixed/Hard/Soft gate and mapping cases exact.')
