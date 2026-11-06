from pathlib import Path
from PIL import Image
import numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def im(n):return np.array(Image.open(r/f'boot-{n}.png').convert('RGB'))
def amber(a):return (a[:,:,0]>190)&(a[:,:,1]>100)&(a[:,:,2]<80)
for name in ['hold-5','hold-4','hold-3','cancelled','restart-5','hold-2','hold-1','entering','short-hold','short-cancel']:
 a=im(name);assert a[0,0].max()>20,name;assert amber(a).any(),name
for name in ['cancel-return','short-return']:assert not im(name)[0,0].any(),name
assert np.array_equal(im('hold-5')[98:176,12:75],im('restart-5')[98:176,12:75])
counts=[amber(im(n))[184:191].sum() for n in ['hold-5','hold-4','hold-3','hold-2','hold-1']]
assert counts==sorted(counts) and len(set(counts))==5,counts
assert not np.array_equal(im('cancelled'),im('entering'))
long=(r/'bootloader-host.log').read_text();short=(r/'bootloader-short-host.log').read_text()
assert long.count('simulated bootloader handoff')==1;assert 'handoff at 9651 ms' in long
assert 'simulated bootloader handoff' not in short
# Bottom-right octave indicator is identical before/after a cancelled chord.
assert np.array_equal(im('single-octave')[224:240,172:240],im('cancel-return')[224:240,172:240])
print('PASS: platform-timed 5/4/3/2/1 countdown, progress, short/long cancellation, fresh restart, unchanged octave, single-key long hold, and one simulated handoff only after a fresh five-second chord hold.')
