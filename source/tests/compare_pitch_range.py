"""Validate source parameter domains and widened FM1 exponent-table arithmetic."""
import ctypes,random,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from inspect_firmware import read,words,text
for index,name,lo,hi in [(9,'Coarse tune',-60,60),(42,'Bend range',0,48),(71,'Max voices',1,8)]:
 record=read(0x9d0889e4+16*index,16);flags,a,b,default,ptr,opt=struct.unpack('<HhhhII',record);assert text(ptr)==name and (a,b)==(lo,hi)
lib=ctypes.CDLL(str(R/'native/libpwperformance.so'));lib.pw_performance_increment.argtypes=[ctypes.c_uint32,ctypes.c_int32];lib.pw_performance_increment.restype=ctypes.c_int32
lib.pw_performance_bend.argtypes=[ctypes.c_int,ctypes.c_uint];lib.pw_performance_bend.restype=ctypes.c_float
lib.pw_performance_units.argtypes=[ctypes.c_float];lib.pw_performance_units.restype=ctypes.c_int32
mant=words(0x9d02c72c,1024);rng=random.Random(28)
for i in range(30000):
 base=rng.randrange(3185015,96200120);u=rng.randrange(-7340032,7340033);exponent=u//524288;expected=min(2147418112,(base*mant[(u>>9)&1023])>>(18-exponent));assert lib.pw_performance_increment(base,u)==expected
for semitones in range(49):
 assert lib.pw_performance_bend(-8192,semitones)==-100*semitones
 assert abs(lib.pw_performance_bend(8191,semitones)-8191*semitones*100/8192)<.001
 assert lib.pw_performance_bend(0,semitones)==0
assert lib.pw_performance_bend(-9000,99)==-4800
assert lib.pw_performance_units(-20000)==-7340032 and lib.pw_performance_units(20000)==7340032
for base in [3185015,96200119,0xffffffff]:
 assert lib.pw_performance_increment(base,-7340032)>0
 assert lib.pw_performance_increment(base,7340032)==2147418112
print('PASS: source domains coarse +/-60, bend 0..48, max voices 1..8 (FM1 exposes 1..4); 30000 exact widened source-table conversions across +/-14 octaves, all 49 MIDI bend endpoints and saturation bounds.')
