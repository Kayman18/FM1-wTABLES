#!/usr/bin/env python3
"""Build 14 lossless 16-frame banks from AKWF and user supplied fm-calm packs.

Each source WAV is used once. No inter-frame interpolation is performed.
A 1024-point FFT resampling and harmonic-limit mip chain preserve the original
periodic waveform and avoid aliasing. Phase anchoring makes adjacent AKWF cycles
better behaved when the oscillator interpolates between them at playback.
Requires: Python 3, numpy. Usage: python3 tools/generate_akwf16.py AKWF.zip fm-calm.zip
"""
from __future__ import annotations
import hashlib, io, json, re, struct, sys, wave, zipfile, zlib
from pathlib import Path
import numpy as np
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from codec import encode,decode
BANKS = [
 ('sinharm', 'AKWF_sinharm'),
 ('cello', 'AKWF_cello'),
 ('granular', 'AKWF_granular'),
 ('fmsynth', 'AKWF_fmsynth'),
 ('hdrawn', 'AKWF_hdrawn'),
 ('flute', 'AKWF_flute'),
 ('eorgan', 'AKWF_eorgan'),
 ('bw_blended', 'AKWF_bw_blended'),
 ('ebass', 'AKWF_ebass'),
 ('fm-calm', 'fm-calm'),
 ('karplus-mid', 'karplus-mid'),
 ('karplus-hi', 'karplus-hi'),
 ('bw_squ', 'AKWF_bw_squ'),
 ('pwm-lead', 'pwm-lead')
]
FRAMES, LEVELS, PER_BANK = 16,10,32736
PARTITION_LIMIT = 0x45000
MAGIC=b'PWLC4901'

def get_bank(az,name,folder,archive_name):
    paths=sorted(n for n in az.namelist() if n.startswith(folder+'/') and n.lower().endswith('.wav'))[:FRAMES]
    if len(paths)!=FRAMES: raise ValueError(f'{name}: need {FRAMES} distinct AKWF files, got {len(paths)}')
    frames=[];digests=[]
    for source in paths:
        content=az.read(source)
        with wave.open(io.BytesIO(content),'rb') as f:
            if f.getsampwidth()!=2:raise ValueError(f'{source}: expected PCM16')
            n,c=f.getnframes(),f.getnchannels()
            arr=np.frombuffer(f.readframes(n),dtype='<i2').reshape(-1,c).astype(np.float64).mean(axis=1)
        arr-=arr.mean()
        arr=np.roll(arr,-int(np.argmax(arr)))
        peak=max(float(np.abs(arr).max()),1.0)
        arr*=28000.0/peak
        bins0=np.fft.rfft(arr)
        mip=[]
        for shift in range(LEVELS):
            width=1024>>shift
            bins=np.zeros(width//2+1,dtype=np.complex128)
            count=min(len(bins),len(bins0))
            bins[:count]=bins0[:count]
            bins[0]=0
            # Avoid a complex Nyquist component in irfft of each downsampled mip.
            bins[-1]=complex(bins[-1].real,0)
            samples=np.rint(np.fft.irfft(bins,n=width)*(width/n)).clip(-32768,32767).astype('<i2')
            mip.append(samples.tobytes())
        frames.append(mip)
        digests.append(hashlib.sha256(content).hexdigest())
    raw=b''.join(frames[f][m] for m in range(LEVELS) for f in range(FRAMES))
    if len(raw)!=PER_BANK*2:raise AssertionError((name,len(raw)))
    packed=encode(struct.unpack('<%dh'%PER_BANK,raw))
    if decode(packed,PER_BANK)!=raw:raise AssertionError(f'{name} codec failed')
    return dict(name=name,archive=archive_name,source_directory=folder,files=paths,wav_sha256=digests,decoded_bytes=len(raw),compressed_bytes=len(packed),decoded_crc32=f'{zlib.crc32(raw):08x}'),raw,packed

def pack_set(banks):
    header=bytearray(512);header[:8]=MAGIC
    struct.pack_into('<II',header,8,1,PER_BANK)
    payload=bytearray()
    for i,(_,raw,packed) in enumerate(banks):
        struct.pack_into('<4I',header,16+16*i,512+len(payload),len(packed),zlib.crc32(raw),zlib.crc32(packed))
        payload.extend(packed)
    struct.pack_into('<I',header,480,512+len(payload))
    struct.pack_into('<I',header,484,zlib.crc32(payload))
    struct.pack_into('<I',header,488,len(banks))
    struct.pack_into('<I',header,508,zlib.crc32(header[:508]))
    return bytes(header+payload)

def validate(data,expected_banks):
    if len(data)<512 or data[:8]!=MAGIC:raise AssertionError('format')
    if struct.unpack_from('<II',data,8)!=(1,PER_BANK):raise AssertionError('geometry')
    if struct.unpack_from('<I',data,488)[0]!=len(expected_banks):raise AssertionError('count')
    if struct.unpack_from('<I',data,480)[0]!=len(data):raise AssertionError('size')
    if zlib.crc32(data[512:])!=struct.unpack_from('<I',data,484)[0]:raise AssertionError('payload crc')
    if zlib.crc32(data[:508])!=struct.unpack_from('<I',data,508)[0]:raise AssertionError('header crc')
    off=512
    for i,(_,raw,packed) in enumerate(expected_banks):
        start,size,dec_crc,cmp_crc=struct.unpack_from('<4I',data,16+16*i)
        if start!=off or data[start:start+size]!=packed or zlib.crc32(raw)!=dec_crc or zlib.crc32(packed)!=cmp_crc:raise AssertionError('index or checksum')
        if decode(data[start:start+size],PER_BANK)!=raw:raise AssertionError('roundtrip mismatch')
        off+=size
    if off!=len(data):raise AssertionError('trailing bytes')


def main(akwf_path,calm_path):
    banks=[]
    with zipfile.ZipFile(akwf_path) as az, zipfile.ZipFile(calm_path) as cz:
        for name,folder in BANKS:
            if folder.startswith('AKWF_'):
                banks.append(get_bank(az,name,folder,'AKWF.zip'))
            else:
                banks.append(get_bank(cz,name,folder,'fm-calm.zip'))
    ext=pack_set(banks[:7]);internal=pack_set(banks[7:]);validate(ext,banks[:7]);validate(internal,banks[7:])
    if len(ext)>PARTITION_LIMIT:raise SystemExit(f'External partition overflow {len(ext)} > {PARTITION_LIMIT}')
    (ROOT/'data/factory-mips.pwlc').write_bytes(ext)
    (ROOT/'data/internal-banks.pwlc').write_bytes(internal)
    layout=f'''/* Generated by tools/generate_akwf16.py. */\n#pragma once\n#define PW_TABLE_FLASH_START 0x97000u\n#define PW_TABLE_BYTES {len(ext)}u\n#define PW_TABLE_FILE_CRC 0x{zlib.crc32(ext):08x}u\n#define PW_FACTORY_BANKS 7u\n#define PW_VIRUS_BANKS 7u\n#define PW_TABLE_BANKS 14u\n#define PW_FRAME_COUNT 16u\n#define PW_BANK_SAMPLES 32736u\n#define PW_FRAME_POS_MAX 3839u\n#define PW_VIRUS_BYTES {len(internal)}u\n#define PW_VIRUS_CRC 0x{zlib.crc32(internal):08x}u\n'''
    (ROOT/'fm1/app/pw_table_layout.h').write_text(layout)
    # Retain pw_virus_data symbol for integration compatibility; now holds 7 AKWF internal banks.
    head='/* Generated AKWF16 internal bank data; no Access Virus waveforms remain. */\n#pragma once\nstatic const uint8_t pw_virus_data[PW_VIRUS_BYTES] __attribute__((aligned(4)))={\n'
    hex_lines=[','.join(f'0x{b:02x}' for b in internal[i:i+24])+',' for i in range(0,len(internal),24)]
    (ROOT/'fm1/app/pw_virus_data.h').write_text(head+'\n'.join(hex_lines)+'\n};\n')
    report={'description':'14 banks of 16 original WAV single cycles from AKWF and fm-calm archives; no fabricated in-between frames. Wav cycles DC-removed, phase-anchored and resampled into 10 Fourier band-limited mipmaps.','geometry':{'banks':14,'frames_per_bank':16,'samples_per_full_rate_frame':1024,'mip_levels':10,'decoded_bytes_per_bank':PER_BANK*2},'external':{'bank_names':[x[0] for x in BANKS[:7]],'bytes':len(ext),'limit':PARTITION_LIMIT,'headroom':PARTITION_LIMIT-len(ext),'sha256':hashlib.sha256(ext).hexdigest()},'internal':{'bank_names':[x[0] for x in BANKS[7:]],'bytes':len(internal),'sha256':hashlib.sha256(internal).hexdigest()},'banks':[p for p,_,_ in banks]}
    (ROOT/'data/akwf16-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print('EXTERNAL',len(ext),'/',PARTITION_LIMIT,'free',PARTITION_LIMIT-len(ext))
    print('INTERNAL',len(internal),'bytes')
    for i,(p,_,_) in enumerate(banks):print(f"{i:02d} {p['name']:<10} {p['compressed_bytes']:>6}  {p['archive']}:{p['source_directory']}")
    print('PASS: 224 distinct original single-cycle WAVs (AKWF + fm-calm); all 14 banks losslessly decode')

if __name__=='__main__':
 if len(sys.argv)!=3:raise SystemExit('Usage: python3 tools/generate_akwf16.py AKWF.zip fm-calm.zip')
 main(sys.argv[1],sys.argv[2])
