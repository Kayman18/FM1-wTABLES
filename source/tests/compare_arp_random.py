"""Compare random selections and state with original MIPS."""
exec((__import__('pathlib').Path(__file__).parent/'compare_arp.py').read_text().split('rng=random.Random(30)')[0])
class State(ctypes.Structure):_fields_=[('seed',ctypes.c_uint32),('used',ctypes.c_uint32*2),('previous',ctypes.c_uint8)]
lib.pw_arp_random_next.argtypes=[ctypes.POINTER(State),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
cases=0
for mode in range(8,11):
 for initial_count in [1,2,3,4,8,16]:
  for octaves in range(1,4):
   for seed in [0,1,0xffffffff,0x12345678]:
    s=State(seed,(ctypes.c_uint32*2)(0,0),255)
    u.mem_write(0x30000,struct.pack('<I',seed)+b'\xff'+bytes(123));u.mem_write(0x40000,b'\x01\x00'+bytes(10))
    for k in range(200):
     count=initial_count if k<100 else max(1,initial_count-1)
     notes=list(reversed([24+i*3 for i in range(count)]));u.mem_write(0x20000,bytes(notes))
     for reg,val in [('A0',0x80020000),('A1',count),('A2',0),('A3',0x80030000),('SP',0x800f0000),('RA',0x9d1fff00)]:u.reg_write(getattr(M,'UC_MIPS_REG_'+reg),val)
     u.mem_write(0xf0010,struct.pack('<III',mode+3,octaves,0x80040000));u.emu_start(0x9d07f804,0,count=20000)
     assert u.reg_read(M.UC_MIPS_REG_PC)==0x9d1fff00
     idx=lib.pw_arp_random_next(ctypes.byref(s),count,mode,octaves)
     expected=sorted(notes)[idx%count]+12*(idx//count)
     assert u.reg_read(M.UC_MIPS_REG_V0)==expected,(mode,count,octaves,k,idx)
     assert int.from_bytes(u.mem_read(0x30000,4),'little')==s.seed
     assert u.mem_read(0x30004,1)[0]==s.previous
     assert list(struct.unpack('<II',u.mem_read(0x30008,8)))==list(s.used)
     cases+=1
print(f'PASS: random arpeggio source: {cases} exact original outputs, PRNG, previous-index and visited-mask states; three modes, four seeds, 1..3 octaves, changing pools.')
