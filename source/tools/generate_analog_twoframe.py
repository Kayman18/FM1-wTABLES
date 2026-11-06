#!/usr/bin/env python3
"""Append eleven genuine two-frame Analog tables to the existing seven internal banks.

The external version1 container and all original compressed bank payloads remain
byte-for-byte unchanged. Mixed version2 uses frame_count-1 nibbles at header492.
The longer 31-frame PWM WAV is deliberately excluded from this build.
"""
from __future__ import annotations
import argparse,hashlib,json,struct,sys,zlib,zipfile
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from codec import encode,decode

MAGIC=b'PWLC4901'
MAX_SAMPLES=32736
SAMPLES_PER_FRAME=2046
ORIGINAL_EXTERNAL_SHA='d0230b91df9329be0d01bd9f9eb8c6969c42a27d2b976af3a5446448204d4d33'
ORIGINAL_INTERNAL_SHA='1d940545da992cbe0dee6c6e74bd7e78b18978fabd25a595a3f56032bf0021b1'
SOURCES=[
 ('ESW Analog - ARP Square.wav','AN ARP SQ'),
 ('ESW Analog - ARP Triangle.wav','AN ARP TRI'),
 ('ESW Analog - JP800 Saw.wav','AN JP800 SAW'),
 ('ESW Analog - Jupiter 8 Saw.wav','AN JUP8 SAW'),
 ('ESW Analog - Jupiter 8 Square.wav','AN JUP8 SQ'),
 ('ESW Analog - Korg Pro Saw.wav','AN KORG SAW'),
 ('ESW Analog - Moog Saw 01.wav','AN MOOG SAW'),
 ('ESW Analog - Moog Square 01 .wav','AN MOOG SQ'),
 ('ESW Analog - SH101 Saw.wav','AN SH101 SAW'),
 ('ESW Analog - SH101 Square 1.wav','AN SH101 SQ1'),
 ('ESW Analog - SH101 Square 2.wav','AN SH101 SQ2'),
]

def sha(b):return hashlib.sha256(b).hexdigest()

def unpack_container(data):
    if len(data)<512 or data[:8]!=MAGIC:raise ValueError('container magic')
    version,maximum=struct.unpack_from('<II',data,8)
    total,payload_crc,count=struct.unpack_from('<III',data,480)
    if version not in (1,2) or maximum!=MAX_SAMPLES:raise ValueError('container geometry')
    if total!=len(data) or not 1<=count<=29:raise ValueError('container size/count')
    if zlib.crc32(data[:508])!=struct.unpack_from('<I',data,508)[0] or zlib.crc32(data[512:])!=payload_crc:raise ValueError('container CRC')
    banks=[];end=512
    for i in range(count):
        off,size,raw_crc,packed_crc=struct.unpack_from('<4I',data,16+i*16)
        frames=16 if version==1 else 1+((data[492+i//2]>>(4*(i&1)))&15)
        if not 2<=frames<=16 or off!=end or size>len(data)-off:raise ValueError('bank range/geometry')
        packed=data[off:off+size]
        if zlib.crc32(packed)!=packed_crc:raise ValueError('bank packed CRC')
        raw=decode(packed,frames*SAMPLES_PER_FRAME)
        if zlib.crc32(raw)!=raw_crc:raise ValueError('bank decoded CRC')
        banks.append({'frames':frames,'raw':raw,'packed':packed});end=off+size
    if end!=len(data):raise ValueError('container trailing bytes')
    return version,banks

def pack_container(banks,version):
    if not 1<=len(banks)<=29 or version not in (1,2):raise ValueError('version/count')
    header=bytearray(512);header[:8]=MAGIC
    struct.pack_into('<II',header,8,version,MAX_SAMPLES)
    payload=bytearray()
    for i,bank in enumerate(banks):
        frames=bank['frames'];raw=bank['raw'];packed=bank['packed']
        if not 2<=frames<=16 or len(raw)!=frames*SAMPLES_PER_FRAME*2:raise ValueError('bank geometry')
        if version==1 and frames!=16:raise ValueError('legacy requires16frames')
        struct.pack_into('<4I',header,16+i*16,512+len(payload),len(packed),zlib.crc32(raw),zlib.crc32(packed))
        payload.extend(packed)
        if version==2:header[492+i//2]|=(frames-1)<<(4*(i&1))
    struct.pack_into('<III',header,480,512+len(payload),zlib.crc32(payload),len(banks))
    struct.pack_into('<I',header,508,zlib.crc32(header[:508]))
    return bytes(header+payload)

def read_wav(blob):
    if blob[:4]!=b'RIFF' or blob[8:12]!=b'WAVE' or struct.unpack_from('<I',blob,4)[0]+8!=len(blob):raise ValueError('WAV header')
    pos=12;fmt=None;audio=None;clm=None
    while pos+8<=len(blob):
        tag=blob[pos:pos+4];size=struct.unpack_from('<I',blob,pos+4)[0];value=blob[pos+8:pos+8+size]
        if len(value)!=size:raise ValueError('truncated WAV chunk')
        if tag==b'fmt ':fmt=struct.unpack_from('<HHIIHH',value)
        elif tag==b'data':audio=value
        elif tag==b'clm ':clm=value
        pos+=8+size+(size&1)
    if fmt!=(3,1,44100,176400,4,32) or clm is None or not clm.startswith(b'<!>2048 '):raise ValueError('source must be mono2048-framefloatWAV')
    if audio is None or len(audio)!=4096*4:raise ValueError('source must contain exactly two frames')
    arr=np.frombuffer(audio,dtype='<f4').astype(np.float64).reshape(2,2048)
    if not np.isfinite(arr).all():raise ValueError('nonfinite source sample')
    return arr

def floating_mips(clean,gain):
    out=[]
    for frame in clean:
        spectrum=np.fft.rfft(frame*gain);levels=[]
        for shift in range(10):
            width=1024>>shift;bins=np.zeros(width//2+1,dtype=np.complex128)
            count=min(len(bins),len(spectrum));bins[:count]=spectrum[:count]
            bins[0]=0;bins[-1]=complex(bins[-1].real,0)
            levels.append(np.fft.irfft(bins,n=width)*(width/2048))
        out.append(levels)
    return out

def convert(blob):
    original=read_wav(blob);dc=original.mean(axis=1);clean=original-dc[:,None]
    peak=float(np.abs(clean).max())
    if peak<=0:raise ValueError('silent source')
    initial_gain=28000.0/peak;gain=initial_gain
    floating=floating_mips(clean,gain)
    initial_mip_peak=max(float(np.abs(x).max()) for frame in floating for x in frame)
    # Filtering may overshoot source peaks. One common per-file adjustment keeps
    # the two frames' relative levels and all mip levels consistent, without clip.
    if initial_mip_peak>32760.0:
        gain*=32760.0/initial_mip_peak
        floating=floating_mips(clean,gain)
    samples=[[np.rint(x) for x in frame] for frame in floating]
    if any(np.any((x < -32768)|(x > 32767)) for frame in samples for x in frame):raise AssertionError('mip conversion clips')
    frames=[[x.astype('<i2').tobytes() for x in frame] for frame in samples]
    raw=b''.join(frames[f][m] for m in range(10) for f in range(2))
    assert len(raw)==8184
    packed=encode(struct.unpack('<4092h',raw))
    assert decode(packed,4092)==raw
    return {'frames':2,'raw':raw,'packed':packed},{
      'source_dc_removed':dc.tolist(),'initial_common_gain':initial_gain,
      'final_common_gain':gain,'gain_adjustment_ratio':gain/initial_gain,
      'initial_floating_mip_peak':initial_mip_peak,
      'final_quantized_peak':max(float(np.abs(x).max()) for frame in samples for x in frame),
      'clipped_samples':0,'phase_policy':'original source frame alignment retained',
      'normalization':'common gain per two-frame source; preserve relative level; reduce together only if mip overshoot would clip'}

def run(archive,validation_report=None):
    external_path=ROOT/'data/factory-mips.pwlc';internal_path=ROOT/'data/internal-banks.pwlc'
    external=external_path.read_bytes();internal_before=internal_path.read_bytes()
    if sha(external)!=ORIGINAL_EXTERNAL_SHA:raise ValueError('unexpected external baseline')
    ext_version,ext_banks=unpack_container(external)
    version,current=unpack_container(internal_before)
    if ext_version!=1 or len(ext_banks)!=7 or len(current) not in (7,18):raise ValueError('unexpected baseline bank counts')
    old=current[:7]
    original_internal=pack_container(old,1)
    if sha(original_internal)!=ORIGINAL_INTERNAL_SHA:raise ValueError('original seven internal payloads differ')
    added=[];rows=[]
    with zipfile.ZipFile(archive) as z:
        for i,(name,label) in enumerate(SOURCES):
            blob=z.read(name);bank,details=convert(blob);added.append(bank)
            rows.append({'bank_id':14+i,'internal_index':7+i,'source':name,'label':label,
              'source_sha256':sha(blob),'frames':2,'decoded_samples':4092,'decoded_bytes':8184,
              'packed_bytes':len(bank['packed']),'decoded_sha256':sha(bank['raw']),
              'packed_sha256':sha(bank['packed']),'decoded_crc32':f"{zlib.crc32(bank['raw']):08x}",
              'packed_crc32':f"{zlib.crc32(bank['packed']):08x}",'codec_roundtrip':'PASS',**details})
    internal=pack_container(old+added,2)
    read_version,decoded=unpack_container(internal)
    assert read_version==2 and [b['frames'] for b in decoded]==[16]*7+[2]*11
    assert all(a['packed']==b['packed'] and a['raw']==b['raw'] for a,b in zip(old,decoded[:7]))
    assert internal[16:16+7*16]==original_internal[16:16+7*16]
    assert external_path.read_bytes()==external
    internal_path.write_bytes(internal)
    layout=f'''/* Generated by tools/generate_analog_twoframe.py. */
#pragma once
#define PW_TABLE_FLASH_START 0x97000u
#define PW_TABLE_BYTES {len(external)}u
#define PW_TABLE_FILE_CRC 0x{zlib.crc32(external):08x}u
#define PW_FACTORY_BANKS 7u
#define PW_VIRUS_BANKS 18u
#define PW_TABLE_BANKS 25u
#define PW_FRAME_COUNT 16u
#define PW_BANK_SAMPLES 32736u
#define PW_FRAME_POS_MAX 3839u
#define PW_VIRUS_BYTES {len(internal)}u
#define PW_VIRUS_CRC 0x{zlib.crc32(internal):08x}u
'''
    (ROOT/'fm1/app/pw_table_layout.h').write_text(layout)
    header='/* Generated mixed internal banks: seven retained16-frame banks and eleven2-frame Analog banks. */\n#pragma once\nstatic const uint8_t pw_virus_data[PW_VIRUS_BYTES] __attribute__((aligned(4)))={\n'
    lines=[','.join(f'0x{b:02x}' for b in internal[i:i+24])+',' for i in range(0,len(internal),24)]
    (ROOT/'fm1/app/pw_virus_data.h').write_text(header+'\n'.join(lines)+'\n};\n')
    labels='/* Generated by tools/generate_analog_twoframe.py. Logical IDs14..24. */\n#pragma once\n#define PW_ANALOG_BANK_FIRST 14u\n#define PW_ANALOG_BANK_COUNT 11u\n#define PW_ANALOG_BANK_LABELS '
    labels+=chr(92)+'\n'
    labels+=(', '+chr(92)+'\n').join(' "'+label+'"' for _,label in SOURCES)+'\n'
    (ROOT/'fm1/app/pw_analog_bank_names.h').write_text(labels)
    report={'purpose':'Generated source assets for mixed-frame test build; no firmware image made by this generator',
      'generator_sha256':sha(Path(__file__).read_bytes()),'codec_sha256':sha((ROOT/'tools/codec.py').read_bytes()),
      'archive_sha256':sha(Path(archive).read_bytes()),'excluded_source':"ESW Analog - 80's PWM.wav",
      'external':{'version':1,'banks':7,'bytes':len(external),'sha256':sha(external),'unchanged':True},
      'original_internal':{'version':1,'banks':7,'bytes':len(original_internal),'sha256':sha(original_internal)},
      'internal':{'version':2,'banks':18,'bytes':len(internal),'sha256':sha(internal),'frame_counts':[16]*7+[2]*11,'new_payload_bytes':sum(len(b['packed']) for b in added),'original_payloads_and_directory_entries_unchanged':True},
      'format':{'magic':MAGIC.decode(),'header_bytes':512,'max_decoded_samples':32736,'directory_entry_bytes':16,'frame_count_nibbles_offset':492,'nibble_value':'frame_count-1; even bank low nibble; odd bank high nibble','accepted_frame_range':[2,16]},
      'banks':rows,'validation':'PASS: all bank CRCs, exact lossless roundtrip, old compressed bytes and old directory entries preserved; zero clipped new mip samples'}
    report_text=json.dumps(report,indent=2)+'\n'
    (ROOT/'data/analog-twoframe-manifest.json').write_text(report_text)
    if validation_report:
        path=Path(validation_report);path.parent.mkdir(parents=True,exist_ok=True);path.write_text(report_text)
    print('EXTERNAL unchanged:',len(external),'bytes; seven banks')
    print('INTERNAL mixedv2:',len(internal),'bytes;18banks; added',report['internal']['new_payload_bytes'],'bytes')
    for row in rows:print(row['bank_id'],row['label'],row['packed_bytes'],'bytes','gainratio',f"{row['gain_adjustment_ratio']:.8f}")
    print(report['validation'])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive',type=Path);parser.add_argument('--validation-report',type=Path)
    args=parser.parse_args();run(args.archive,args.validation_report)
