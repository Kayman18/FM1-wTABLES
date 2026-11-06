"""Full original -8ve routine and chord descriptor flags, no arithmetic stubs."""
exec((__import__('pathlib').Path(__file__).parent/'compare_arp.py').read_text().split('rng=random.Random(30)')[0])
lib.pw_arp_octave_next.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
lib.pw_chord_omit_octave.argtypes=[ctypes.c_uint]
for shape in range(14):assert lib.pw_chord_omit_octave(shape)==((read(0x9d082d40+12*shape,2)[1])&1)
assert [text(x) for x in words(0x9d0827e4+5*4,4)]==['Up -8ve','Down -8ve','Alt -8ve','Alt2 -8ve']
cases=0
for mode in range(11,15):
 for count in [1,2,3,4,5,8,16]:
  for octaves in range(1,4):
   for flag in [0,1]:
    if count==1 and flag:continue # actual flagged source shapes always have at least three members
    for variant in range(3):
     notes=[24+i*3 for i in range(count)]
     if variant==1:notes.reverse()
     if variant==2:notes=[48]*count
     u.mem_write(0x20000,bytes(notes));u.mem_write(0x30000,bytes(128));u.mem_write(0x30004,b'\xff');u.mem_write(0x40000,bytes([count,flag])+bytes(10));step=ctypes.c_uint8(255)
     for k in range(100):
      for reg,val in [('A0',0x80020000),('A1',count),('A2',0),('A3',0x80030000),('SP',0x800f0000),('RA',0x9d1fff00)]:u.reg_write(getattr(M,'UC_MIPS_REG_'+reg),val)
      u.mem_write(0xf0010,struct.pack('<III',mode-6,octaves,0x80040000));u.emu_start(0x9d07f804,0,count=20000)
      assert u.reg_read(M.UC_MIPS_REG_PC)==0x9d1fff00
      index=lib.pw_arp_octave_next(ctypes.byref(step),count,mode,octaves,flag)
      expected=sorted(notes)[index%count]+12*(index//count)
      assert u.reg_read(M.UC_MIPS_REG_V0)==expected,(mode,count,octaves,flag,k,index,u.reg_read(M.UC_MIPS_REG_V0),expected)
      assert u.mem_read(0x30004,1)[0]==step.value,(mode,count,octaves,flag,k,'step')
      cases+=1
print(f'PASS: octave-omission source: {cases} exact original outputs and step states; all four modes, three octave ranges, both descriptor flags, sorted/unsorted/duplicate inputs; all 14 chord flags matched.')
