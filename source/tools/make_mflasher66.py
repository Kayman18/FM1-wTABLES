#!/usr/bin/env python3
"""Build verified 1 MiB M-Flasher image with legacy and variable-frame banks.

ONLY a known full 1 MiB source backup with matching original SDK contents
may be used; calibrations, boot regions, identity and private settings remain
byte-for-byte. Do not use the app-only .fwsc for a clean install, because it
does not install the matching 7 external AKWF banks.
"""
from pathlib import Path
import hashlib,os,re,struct,sys,zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'fm1/base/tools'))
import fm1pkg_make as f
from fm1pkg_make import crc16,enc,chipkey_decode,sfc,flash_image
f.SDK=Path(os.environ.get('AC79_SDK',str(ROOT/'fm1/sdk')))
START,REGION=0x97000,0x45000
MAX_APP=581564

def entry(raw,offset):
    d=bytearray(raw[offset:offset+32]);enc(d,0,32)
    if crc16(d[2:])!=struct.unpack_from('<H',d)[0]:raise ValueError('invalid backup directory')
    return d

def verify_banks(data,expected):
    if len(data)<512 or data[:8]!=b'PWLC4901':raise ValueError('bad bank magic')
    version,max_samples=struct.unpack_from('<II',data,8)
    if version not in (1,2) or max_samples!=32736:raise ValueError('bad bank format or cache geometry')
    if not 1<=expected<=29:raise ValueError('bank count exceeds header capacity')
    if struct.unpack_from('<I',data,480)[0]!=len(data):raise ValueError('bad bank length')
    if struct.unpack_from('<I',data,488)[0]!=expected:raise ValueError('bad bank count')
    if zlib.crc32(data[512:])!=struct.unpack_from('<I',data,484)[0]:raise ValueError('payload CRC')
    if zlib.crc32(data[:508])!=struct.unpack_from('<I',data,508)[0]:raise ValueError('header CRC')
    from codec import decode
    off=512
    for i in range(expected):
        frames=16 if version==1 else ((data[492+i//2]>>((i&1)*4))&15)+1
        if not 2<=frames<=16:raise ValueError('invalid bank frame count')
        pos,size,decoded_crc,packed_crc=struct.unpack_from('<4I',data,16+16*i)
        if pos!=off or size<1 or size>len(data)-pos:raise ValueError('bad bank offset')
        packed=data[pos:pos+size]
        if zlib.crc32(packed)!=packed_crc:raise ValueError('packed CRC')
        raw=decode(packed,frames*2046)
        if zlib.crc32(raw)!=decoded_crc or len(raw)!=frames*4092:raise ValueError('decoded CRC/length')
        off+=size
    if off!=len(data):raise ValueError('bad packed trailing bytes')

def configured_bank_count(name):
    layout=(ROOT/'fm1/app/pw_table_layout.h').read_text()
    match=re.search(r'^#define\s+'+re.escape(name)+r'\s+(\d+)u?\s*$',layout,re.M)
    if not match:raise ValueError('missing generated bank count: '+name)
    return int(match.group(1))

def make(base,app,partition,internal):
    if len(base)!=1048576:raise ValueError('expected 1 MiB backup')
    h=entry(base,0);isd=entry(base,0x40)
    if isd[16:30]!=b'isd_config.ini':raise ValueError('isd_config.ini not found')
    off,n=struct.unpack_from('<II',isd,4);raw=base[off:off+n]
    if crc16(raw)!=struct.unpack_from('<H',isd,2)[0]:raise ValueError('invalid ini CRC')
    key=chipkey_decode(raw[:32])
    if struct.unpack_from('<I',entry(base,0x60),4)[0]!=0x4000:raise ValueError('incorrect app offset')
    verify_banks(partition,configured_bank_count('PW_FACTORY_BANKS'))
    verify_banks(internal,configured_bank_count('PW_VIRUS_BANKS'))
    if len(partition)>REGION:raise ValueError('factory partition overflow')
    if len(app)>MAX_APP:raise ValueError('app slot overflow')
    if app.count(internal)!=1:raise ValueError('the compiled app must contain exactly one internal bank image')
    generated=flash_image(app,key)
    image=bytearray(base)
    image[0x4000:0x93000]=generated[0x4000:0x93000]
    image[START:START+REGION]=partition+b'\xff'*(REGION-len(partition))
    if image[0xdc000:0xde000]!=base[0xdc000:0xde000]:raise ValueError('protected region changed')
    # Check firmware is fully decryptable with the actual preserved chip key.
    decrypted=bytearray(image[0x4000:0x93000]);sfc(decrypted,0,len(decrypted),0,key)
    if crc16(decrypted[2:32])!=struct.unpack_from('<H',decrypted)[0]:raise ValueError('app header CRC')
    blk=struct.unpack_from('<I',decrypted,8)[0]
    if crc16(decrypted[32:blk])!=struct.unpack_from('<H',decrypted,2)[0]:raise ValueError('dir CRC')
    a=decrypted[32:64]
    if crc16(a[2:])!=struct.unpack_from('<H',a)[0]:raise ValueError('dir entry CRC')
    pos,size=struct.unpack_from('<II',a,4)
    if decrypted[pos:pos+len(app)]!=app or crc16(decrypted[pos:pos+size])!=struct.unpack_from('<H',a,2)[0]:raise ValueError('app bytes/CRC')
    if image[:0x4000]!=base[:0x4000] or image[0x93000:START]!=base[0x93000:START] or image[START+REGION:]!=base[START+REGION:]:raise ValueError('outside managed sectors changed')
    if image[START:START+len(partition)]!=partition:raise ValueError('factory partition mismatch')
    return bytes(image)

if __name__=='__main__':
    if len(sys.argv)!=3:raise SystemExit('Usage: python3 tools/make_mflasher66.py EXISTING-1M.bin OUTPUT-1M.bin')
    base=Path(sys.argv[1]).read_bytes();app=(ROOT/'fm1/app/build/pwcore.bin').read_bytes()
    p=(ROOT/'data/factory-mips.pwlc').read_bytes();i=(ROOT/'data/internal-banks.pwlc').read_bytes()
    result=make(base,app,p,i)
    target=Path(sys.argv[2]);target.write_bytes(result)
    print('PASS 1 MiB: target compiler image + configured banks, full decrypted app/CRC and protection verification')
    print('App:',len(app),'bytes; external:',len(p),'bytes; internal:',len(i),'bytes')
    print('SHA256:',hashlib.sha256(result).hexdigest())
