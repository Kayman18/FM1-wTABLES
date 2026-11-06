import sys,ctypes,struct,random
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop_at=0
u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:u.emu_stop() if a==stop_at else None)
lib=ctypes.CDLL(str(R/'native/libpwunison.so'));lib.pw_unison_detune.argtypes=[ctypes.c_uint]*3;lib.pw_unison_detune.restype=ctypes.c_int32;lib.pw_unison_wave.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_float];lib.pw_unison_wave.restype=ctypes.c_float
rng=random.Random(126118);worst=0
for case in range(8000):
 count=rng.randrange(1,9);member=rng.randrange(count);amount=rng.randrange(101)
 u.mem_write(0x6edf4,struct.pack('<h',amount));u.mem_write(0x6edf2,struct.pack('<h',count));u.reg_write(UC_MIPS_REG_FP,0x80075780);u.reg_write(UC_MIPS_REG_SP,0x80030000);stop_at=0x9d09ccd8
 u.emu_start(0x9d09ccac,0,count=100);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 edge=struct.unpack('<i',u.mem_read(0x3002c,4))[0];assert edge==436*amount
 u.reg_write(UC_MIPS_REG_S6,count);u.reg_write(UC_MIPS_REG_S1,member*edge*2);stop_at=0x9d09cdd8;u.emu_start(0x9d09cdb4,0,count=100);expected=ctypes.c_int32(u.reg_read(UC_MIPS_REG_A0)).value
 assert lib.pw_unison_detune(count,member,amount)==expected
 slot=rng.randrange(8);spread=rng.randrange(-100,101);u.mem_write(0x6ed88,struct.pack('<h',spread));u.mem_write(0x6edae,bytes(2));u.mem_write(0x6edb2,bytes(2));u.reg_write(UC_MIPS_REG_A0,slot);u.reg_write(UC_MIPS_REG_RA,0x9d000000);stop_at=0x9d000000
 u.emu_start(0x9d0ee244,0,count=150);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 expected=struct.unpack('<f',u.mem_read(0x53128+slot*128,4))[0];actual=lib.pw_unison_wave(slot,8,spread);err=abs(expected-actual);worst=max(worst,err);assert err<1e-7,(slot,spread,expected,actual)
print(f'PASS: 8000 original MIPS unison integer detune distributions and isolated wave-spread helper cases; count1..8, signed spread, all slots. Detune exact; normalized wave error {worst:.3g}.')
