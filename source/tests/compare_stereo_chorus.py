import sys,ctypes,struct,random,numpy as np
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read
from unicorn import *
from unicorn.mips_const import *
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.ctl_set_cpu_model(UC_CPU_MIPS32_74KF);u.mem_map(0,0x100000);u.mem_map(0x1d000000,0x200000);u.mem_write(0x1d000000,read(0x9d000000,0x1b0000));u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
stop=0x9d15e7a0;loads=0
regints={0x9d02a38c:(UC_MIPS_REG_V1,UC_MIPS_REG_V0,UC_MIPS_REG_V1,4),0x9d02a3c8:(UC_MIPS_REG_T3,UC_MIPS_REG_V1,UC_MIPS_REG_V0,1),0x9d02a3cc:(UC_MIPS_REG_T2,UC_MIPS_REG_A3,UC_MIPS_REG_V0,1),0x9d02a3d0:(UC_MIPS_REG_A0,UC_MIPS_REG_T0,UC_MIPS_REG_V0,1),0x9d02a3d4:(UC_MIPS_REG_V0,UC_MIPS_REG_A1,UC_MIPS_REG_V0,1)}
regfloat={0x9d02a48c:(UC_MIPS_REG_F15,UC_MIPS_REG_V1),0x9d02a490:(UC_MIPS_REG_F14,UC_MIPS_REG_A3),0x9d02a494:(UC_MIPS_REG_F13,UC_MIPS_REG_T0),0x9d02a498:(UC_MIPS_REG_F8,UC_MIPS_REG_A1)}
def hook(u,a,s,d):
 global loads
 if a==stop:u.emu_stop();return
 if a in regints:
  dest,idx,base,n=regints[a];adr=(u.reg_read(idx)+u.reg_read(base))&0x1fffffff;value=int.from_bytes(u.mem_read(adr,n),'little');u.reg_write(dest,value);u.reg_write(UC_MIPS_REG_PC,a+4);loads+=1
 if a in regfloat:
  dest,idx=regfloat[a];adr=(u.reg_read(idx)+u.reg_read(UC_MIPS_REG_V0))&0x1fffffff;u.reg_write(dest,int.from_bytes(u.mem_read(adr,4),'little'));u.reg_write(UC_MIPS_REG_PC,a+4);loads+=1
u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x9d15e6fc,0,count=100000);assert u.reg_read(UC_MIPS_REG_PC)==stop
z=np.load(R/'validation/chorus-tables.npz');assert np.array_equal(np.frombuffer(u.mem_read(0x57f80,2048),np.uint8).reshape(2,1024),z['index48']);assert np.array_equal(np.frombuffer(u.mem_read(0x58780,8192),'<f4').reshape(2,1024),z['fraction48'])
class Chorus(ctypes.Structure):_fields_=[('ring',ctypes.c_float*1024),('phase',ctypes.c_uint32),('write',ctypes.c_uint32)]
lib=ctypes.CDLL(str(R/'native/libpwstereochorus.so'));lib.pw_chorus_tick.argtypes=[ctypes.POINTER(Chorus),ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_float),ctypes.c_uint,ctypes.c_uint];lib.pw_stereo_gains.argtypes=[ctypes.c_uint,ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_int,ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_float)]
def setf(reg,x):u.reg_write(reg,struct.unpack('<I',struct.pack('<f',x))[0])
def getf(reg):return struct.unpack('<f',struct.pack('<I',u.reg_read(reg)))[0]
def wr(a,x,fmt='<I'):u.mem_write(a&0x1fffffff,struct.pack(fmt,x))
rng=np.random.default_rng(126116);worst=0
for case in range(5000):
 ring=rng.uniform(-1,1,1024).astype('<f4');phase=int(rng.integers(0,1<<32));write=int(rng.integers(0,256));mode=case%2+1;input=rng.uniform(-1,1,4).astype('<f4');state=Chorus();ctypes.memmove(ctypes.addressof(state),ring.ctypes.data,4096);state.phase=phase;state.write=write
 u.mem_write(0x56f70,ring.tobytes()+ring[:4].tobytes());wr(0x56f5c,phase);wr(0x56f60,write)
 for reg,v in [(UC_MIPS_REG_S4,0x80055780),(UC_MIPS_REG_S6,0x80005780),(UC_MIPS_REG_A2,mode)]:u.reg_write(reg,v)
 for reg,x in zip([UC_MIPS_REG_F11,UC_MIPS_REG_F10,UC_MIPS_REG_F12,UC_MIPS_REG_F9],input):setf(reg,float(x))
 stop=0x9d02a4dc;u.emu_start(0x9d02a32c,0,count=200);assert u.reg_read(UC_MIPS_REG_PC)==stop,hex(u.reg_read(UC_MIPS_REG_PC))
 expected=[getf(reg) for reg in [UC_MIPS_REG_F11,UC_MIPS_REG_F10,UC_MIPS_REG_F12,UC_MIPS_REG_F9]]
 inp=(ctypes.c_float*4)(*input);out=(ctypes.c_float*4)();lib.pw_chorus_tick(ctypes.byref(state),inp,out,mode,48000)
 err=max(abs(a-b) for a,b in zip(expected,out));worst=max(worst,err);assert err<8e-7,(case,mode,expected,list(out),err)
 assert state.phase==int.from_bytes(u.mem_read(0x56f5c,4),'little');assert state.write==int.from_bytes(u.mem_read(0x56f60,4),'little');assert bytes(state.ring)==bytes(u.mem_read(0x56f70,4096))
# Original voice/pair panning callback computes all eight slot gains.
rand=random.Random(126116);worst_pan=0
for case in range(2000):
 spread=rand.randrange(-100,101);mode=case%2+1;u.reg_write(UC_MIPS_REG_V0,spread&0xffffffff);u.reg_write(UC_MIPS_REG_A3,mode-1);stop=0x9d0658c4
 u.emu_start(0x9d065bac,0,count=500);assert u.reg_read(UC_MIPS_REG_PC)==stop
 for slot in range(8):
  expected=struct.unpack('<2f',u.mem_read(0x530d8+slot*128,8));left=ctypes.c_float();right=ctypes.c_float();lib.pw_stereo_gains(mode,spread,slot,8,48,ctypes.byref(left),ctypes.byref(right));err=max(abs(expected[0]-left.value),abs(expected[1]-right.value));worst_pan=max(worst_pan,err);assert err<1e-6,(case,slot,expected,left.value,right.value)
for case in range(3000):
 spread=rand.randrange(-100,101);note=rand.randrange(24,84);slot=rand.randrange(8);wr(0x6edf6,spread,'<h');u.mem_write(0x53134+slot*128,bytes([note]));u.reg_write(UC_MIPS_REG_A0,slot);u.reg_write(UC_MIPS_REG_RA,0x9d000000);stop=0x9d000000
 u.emu_start(0x9d0e9014,0,count=200);assert u.reg_read(UC_MIPS_REG_PC)==stop
 expected=struct.unpack('<2f',u.mem_read(0x530d8+slot*128,8));left=ctypes.c_float();right=ctypes.c_float();lib.pw_stereo_gains(3,spread,slot,8,note,ctypes.byref(left),ctypes.byref(right));err=max(abs(expected[0]-left.value),abs(expected[1]-right.value));worst_pan=max(worst_pan,err);assert err<1e-6,(case,expected,left.value,right.value)
print(f'PASS: original 2048 chorus table entries, 5000 four-channel chorus states/modes (max error {worst:.3g}), 2000 original eight-slot stereo callbacks plus 3000 pitch-pan cases (max error {worst_pan:.3g}); only {loads} indexed loads hooked, no DSP arithmetic replaced.')
