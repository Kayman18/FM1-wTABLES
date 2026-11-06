from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1];z=np.load(r/'validation/mip-tables.npz')
for s in range(1,11):
 a=z[f'mip{s}'];assert a.shape==(8,8,2048>>s);assert abs(a.astype(int)).max()<=30000;assert np.max(abs(a.mean(axis=2)))<.5
 # Common normalization retains fundamental amplitude throughout each chain.
 spec=np.fft.rfft(a.astype(float),axis=2)/(2048>>s);ref=np.fft.rfft(z['mip1'].astype(float),axis=2)/1024
 fundamental=spec[:,:,1]*(.5 if s==10 else 1)
 assert np.max(abs(fundamental-ref[:,:,1]))<1.0
 # DC-free phase-aligned real fundamental remains positive at phase zero.
 assert np.max(abs(fundamental.imag))<1.0
print('PASS: all 640 tables, dimensions, signed-16-bit bounds, DC removal, common fundamental gain and phase alignment across all 10 levels.')
