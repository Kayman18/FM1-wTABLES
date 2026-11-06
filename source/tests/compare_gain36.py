"""Execute the original dB initializer, powf, gain lookup and delay-level callback."""
import sys,struct,ctypes
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,0x20000000)
stop_at=0
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==stop_at else None)
u.mem_write(0x64a34,bytes([0xa5])*260)
u.reg_write(UC_MIPS_REG_SP,0x800f0000);u.reg_write(UC_MIPS_REG_S6,0x80065780)
stop_at=0x9d15e500;u.emu_start(0x9d15e4b8,0,count=1000000);assert u.reg_read(UC_MIPS_REG_PC)==0x9d15e500
L=ctypes.CDLL(str(R/'native/libtest36engine.so'));L.pw_gain_db.argtypes=[ctypes.c_int];L.pw_gain_db.restype=ctypes.c_float
for db in range(-40,25):
 expected=bytes(u.mem_read(0x64ad4+4*db,4));assert struct.pack('<f',L.pw_gain_db(db))==expected,db
 u.mem_write(0x6edc8,struct.pack('<h',db));u.reg_write(UC_MIPS_REG_A2,0x80075780);u.reg_write(UC_MIPS_REG_V1,0x80005780)
 stop_at=0x9d0ef1a4;u.emu_start(0x9d0ef180,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==0x9d0ef1a4
 assert struct.pack('<I',u.reg_read(UC_MIPS_REG_F1))==expected
for mode in range(3):
 for db in range(-40,1):
  u.mem_write(0x6edea,struct.pack('<hh',mode,db));stop_at=0x9d0658c4;u.emu_start(0x9d065e64,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==0x9d0658c4
  expected=L.pw_gain_db(db) if mode else 0
  assert bytes(u.mem_read(0x56f50,4))==struct.pack('<f',expected),(mode,db)
assert L.pw_gain_db(-127)==0 and L.pw_gain_db(127)==L.pw_gain_db(24)
print('PASS: TEST36 original MIPS gain: all 65 generated dB entries and gain lookups bit-exact; 123 delay mode/return callbacks exact; original powf executed without arithmetic hooks.')
