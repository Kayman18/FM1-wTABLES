"""Compare the isolated source velocity/note tracking calculations."""
import sys,struct,ctypes,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF)
u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop_at=0x9d000000
def stop(u,a,s,d):
 if a==stop_at:u.emu_stop()
u.hook_add(UC_HOOK_CODE,stop)
lib=ctypes.CDLL(str(ROOT/'native/libpwtracking.so'))
for fn in [lib.pw_tracking_wave_offset,lib.pw_tracking_cutoff_offset]:fn.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_float,ctypes.c_float];fn.restype=ctypes.c_float
rng=random.Random(126110);worst_wave=worst_cut=0
for i in range(6000):
 note=rng.randrange(24,84);vel=rng.randrange(1,128);vw=rng.randrange(-100,101);pw=rng.randrange(-100,101);vc=rng.randrange(-127,128);pc=rng.randrange(-100,101)
 u.mem_write(0x5312e,bytes([vel]));u.mem_write(0x53134,bytes([note]))
 for offset,value in [(0x69d2,vw),(0x69ce,pw),(0x69d0,vc),(0x69cc,pc)]:u.mem_write(0x75780-offset,struct.pack('<h',value))
 u.reg_write(UC_MIPS_REG_A0,0);u.reg_write(UC_MIPS_REG_RA,0x9d000000);stop_at=0x9d000000
 u.emu_start(0x9d0ee244,0,count=150);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 wave=struct.unpack('<f',u.mem_read(0x53128,4))[0];actual_wave=lib.pw_tracking_wave_offset(note,vel,vw,pw)
 stop_at=0x9d0658c8;u.emu_start(0x9d065da4,0,count=500);assert u.reg_read(UC_MIPS_REG_PC)==stop_at
 cut=struct.unpack('<f',u.mem_read(0x53124,4))[0];actual_cut=lib.pw_tracking_cutoff_offset(note,vel,vc,pc)
 worst_wave=max(worst_wave,abs(wave-actual_wave));worst_cut=max(worst_cut,abs(cut-actual_cut))
 assert abs(wave-actual_wave)<3e-7 and abs(cut-actual_cut)<2e-5,(i,wave,actual_wave,cut,actual_cut)
print(f'PASS: 6000 signed velocity/pitch tracking cases versus original wave helper and cutoff update loop. Max normalized wave error {worst_wave:.3g}; cutoff error {worst_cut:.3g} semitones. Source unison spread isolated at zero.')
