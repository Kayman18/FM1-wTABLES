"""Trial conversion of supplied Virus recordings. Source recordings are not native tables.
Cycle extraction/resampling is an adaptation; codec compression is lossless thereafter.
"""
from pathlib import Path
import io,wave,zipfile,sys,struct,json,hashlib,zlib
import numpy as np
from scipy.signal import correlate,find_peaks
from scipy.interpolate import CubicSpline
from scipy.optimize import minimize_scalar
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from codec import encode,decode
R=ROOT/'data/virus-conversion';(R/'cycles').mkdir(parents=True,exist_ok=True)
z=zipfile.ZipFile(ROOT/'reference/Access-Virus-C-TI.zip');rows=[];levels=[];features=[];seen={}
for k in range(1,201):
 raw=z.read(f'Access Virus - C_TI/Virus_WT_{k:03}.wav');w=wave.open(io.BytesIO(raw));x=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float);x-=x.mean();digest=hashlib.sha256(raw).hexdigest()
 c=correlate(x,x,method='fft',mode='full')[len(x)-1:];power=np.r_[0,np.cumsum(x*x)];lags=np.arange(min(2048,len(x)//3));norm=c[lags]/np.sqrt(np.maximum(1,(power[-1]-power[lags])*power[len(x)-lags]));peaks=find_peaks(norm)[0];peaks=peaks[peaks>=8];best=max(norm[peaks]);candidates=peaks[norm[peaks]>=max(.975,best-.008)]
 lag=int(candidates[0] if len(candidates) else peaks[np.argmax(norm[peaks])]);spline=CubicSpline(np.arange(len(x)),x)
 # Refine with fractional-delay waveform matching, not an FFT-bin pitch guess.
 t=np.arange(2,len(x)-lag-3,dtype=float);a=x[t.astype(int)];energy=np.dot(a,a)
 def mismatch(p):
  b=spline(t+p);return 1-np.dot(a,b)/np.sqrt(energy*np.dot(b,b))
 fit=minimize_scalar(mismatch,bounds=(lag-.65,lag+.65),method='bounded',options={'xatol':1e-7});period=float(fit.x)
 cycles=max(1,int((len(x)-4)/period)-1);start=(len(x)-cycles*period)*.5
 phase=np.arange(1024)/1024.;y=np.mean(spline(start+(np.arange(cycles)[:,None]+phase)*period),axis=0);y-=y.mean()
 spec=np.fft.rfft(y)/1024;spec[0]=0
 # Remove frequencies above the recording's available Nyquist bandwidth.
 cutoff=min(511,int(period/2));spec[cutoff+1:]=0
 # Set phase from the first available significant harmonic.
 strong=np.where(abs(spec[1:])>np.max(abs(spec))*.01)[0]+1;anchor=int(strong[0]);shift=-np.angle(spec[anchor])/anchor
 spec*=np.exp(1j*np.arange(len(spec))*shift)
 ls=[]
 for n in [1024,512,256,128,64,32,16,8,4,2]:
  sp=np.zeros(n//2+1,dtype=complex);sp[:n//2]=spec[:n//2]*n
  if n==2:sp[1]=2*spec[1].real*n
  ls.append(np.fft.irfft(sp,n=n))
 peak=max(float(np.max(np.abs(v))) for v in ls);ls=[np.rint(v*(28000/peak)).astype('<i2') for v in ls];levels.append(ls)
 with wave.open(str(R/'cycles'/f'Virus_{k:03}_1024.wav'),'wb') as out:out.setnchannels(1);out.setsampwidth(2);out.setframerate(44100);out.writeframes(ls[0].tobytes())
 mag=abs(np.fft.rfft(ls[0]))[1:257];mag/=max(np.linalg.norm(mag),1e-30);features.append(np.sqrt(mag))
 row=dict(id=k,source_frames=len(x),period_samples=period,recording_rate=48000,period_match=1-fit.fun,cycles_averaged=cycles,source_sha256=digest,exact_duplicate_of=seen.get(digest),peak=28000)
 seen.setdefault(digest,k);rows.append(row)
 print(k,'period',round(period,4),'match',round(1-fit.fun,5),flush=True)
(R/'conversion.json').write_text(json.dumps(rows,indent=2)+'\n');np.savez_compressed(R/'converted.npz',**{f'w{i+1}_m{m}':v for i,l in enumerate(levels) for m,v in enumerate(l)})
# Deduplicate identical source files only. Similar sounds remain eligible.
unique=[i for i,r in enumerate(rows) if r['exact_duplicate_of'] is None]
f=np.array(features);f/=np.linalg.norm(f,axis=1)[:,None]
# Rank by measured storage cost, using spectral diversity to break ties.
order=[];remaining=unique.copy();center=np.mean(f[remaining],axis=0)
first=min(remaining,key=lambda i:np.linalg.norm(f[i]-center));order.append(first);remaining.remove(first)
while remaining:
 d=np.min(1-f[remaining]@f[order].T,axis=1)
 pick=remaining[int(np.argmax(d))];order.append(pick);remaining.remove(pick)
def bank(ids):
 ids=list(ids);ids+=ids[-1:]*(8-len(ids));raw=b''.join(levels[i][m].tobytes() for m in range(10) for i in ids);c=encode(struct.unpack('<16368h',raw));assert decode(c)==raw;return raw,c
# Budget includes header; leave an extra 4 KiB of the 212 KiB allowance for loader/UI growth.
BUDGET=208*1024
# Prefer count within the byte budget, then improve diversity without losing count.
costs={i:len(encode(struct.unpack('<2046h',b''.join(levels[i][m].tobytes() for m in range(10))))) for i in unique}
cheap=sorted(unique,key=lambda i:(costs[i],order.index(i)))
chosen=[];groups=[];payload=[];decoded=[]
for pos in range(0,len(cheap),8):
 ids=cheap[pos:pos+8]
 if len(ids)<8:break
 raw,c=bank(ids)
 if 512+sum(map(len,payload))+len(c)>BUDGET:break
 groups.append([i+1 for i in ids]);chosen+=ids;payload.append(c);decoded.append(raw)
# Replace redundant cheap choices with diverse alternatives within the same cap.
for candidate in order:
 if candidate in chosen:continue
 old_total=512+sum(map(len,payload))
 old_diversity=float(np.mean(np.max(f[unique]@f[chosen].T,axis=1)))
 improvements=[]
 for at,previous in enumerate(chosen):
  if costs[candidate]-costs[previous]>BUDGET-old_total+128:continue
  proposal=chosen.copy();proposal[at]=candidate
  score=float(np.mean(np.max(f[unique]@f[proposal].T,axis=1)))
  if score>old_diversity+1e-6:improvements.append((score,at))
 for score,at in sorted(improvements,reverse=True)[:4]:
  proposal=chosen.copy();proposal[at]=candidate;group=at//8
  raw,c=bank(proposal[group*8:group*8+8]);total=old_total-len(payload[group])+len(c)
  if total<=BUDGET:
   chosen=proposal;payload[group]=c;decoded[group]=raw;groups[group]=[i+1 for i in chosen[group*8:group*8+8]]
   break
header=bytearray(512);header[:8]=b'PWLC4901';struct.pack_into('<II',header,8,1,16368);offset=512
for i,(raw,c) in enumerate(zip(decoded,payload)):struct.pack_into('<4I',header,16+i*16,offset,len(c),zlib.crc32(raw),zlib.crc32(c));offset+=len(c)
body=b''.join(payload);struct.pack_into('<III',header,480,offset,zlib.crc32(body),len(groups));struct.pack_into('<I',header,508,zlib.crc32(header[:508]));packed=bytes(header)+body
(R/'virus-banks.pwlc').write_bytes(packed);(R/'virus-reference.raw').write_bytes(b''.join(decoded))
manifest=dict(source_files=200,exact_unique_files=len(unique),budget_bytes=BUDGET,bytes=len(packed),banks=groups,selected_waves=len(chosen),bank_sizes=list(map(len,payload)),unselected=[i+1 for i in order if i not in chosen],minimum_period_match=min(r['period_match'] for r in rows),median_period_match=float(np.median([r['period_match'] for r in rows])))
(R/'selection.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest),flush=True)
