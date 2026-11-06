from pathlib import Path
import zipfile,struct,sys,json,hashlib
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r))
from inspect_firmware import read,text
params=[]
for i in range(78):
 flags,lo,hi,default,pointer,options=struct.unpack('<HhhhII',read(0x9d0889e4+16*i,16))
 params.append(dict(index=i,name=text(pointer),min=lo,max=hi,default=default))
assert params==json.loads((r/'presets/source-parameters.json').read_text())
z=zipfile.ZipFile(r/'presets/PW-Trk-el-son.zip');records=[]
for name in sorted(n for n in z.namelist() if n.endswith('.dexpreset') and not n.startswith('__MACOSX/')):
 b=z.read(name);assert len(b)==1024 and b[:8]==b'DEXBPRST' and struct.unpack_from('<I',b,8)[0]==1
 assert struct.unpack_from('<h',b,0x238)[0]==9
 values=list(struct.unpack_from('<78h',b,0x23c))
 assert all(p['min']<=values[p['index']]<=p['max'] for p in params),(name,values)
 records.append(dict(file=name,name=b[516:532].split(b'\0')[0].decode().strip(),params=values))
assert len(records)==50
(r/'presets/source-decoded.json').write_text(json.dumps(records,indent=2))
for row in json.loads((r/'presets/fm1-presets.json').read_text()):
 assert hashlib.sha256(z.read(row['source_file'])).hexdigest()==row['source_sha256']
print('PASS: 50 preset records / 3900 parameter values within original firmware descriptor domains; 16 source hashes verified.')
