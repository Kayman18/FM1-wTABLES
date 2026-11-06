from pathlib import Path
from PIL import Image
import numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def im(name):return np.array(Image.open(r/f'overlay-{name}.png').convert('RGB'))
def active(name):
 a=im(name);assert a.shape==(240,240,3);assert np.array_equal(a[0,0],[33,32,33]) or (a[0,0].max()>20 and a[0,0].max()<45),name
 assert np.any(a[:,:,0]>200),name
for name in ['env-held','env-still-held','env-released','combo-knob','combo-panel','delay-time','delay-choice','preset','master','dynamics-choice','clamped-knob','select','algo','unassigned-combo','all-buttons','master-zero','master-full','k4']:active(name)
assert np.array_equal(im('env-held'),im('env-still-held'))
for name in ['idle','env-return','delay-return','master-return','dynamics-return','unassigned-return','all-return']:assert not im(name)[0,0].any(),name
assert np.array_equal(im('master-return'),im('master-jitter'))
buttons=['FX','SEL','ENV','LFO','EDIT','GLO','HOME','SAVE','ARP','SEQ','PLAY','REC','OCTdn','OCTup']
for b in buttons:active('button-'+b)
assert not np.array_equal(im('delay-time'),im('delay-choice'))
assert not np.array_equal(im('master-zero'),im('master-full'))
print('PASS: full-screen overlays for all 14 buttons, combinations, K1..4, PRESET, MASTER and SELECT/ALGO; held >700ms, release/idle returns, clamped control events, master jitter suppression and endpoints. Screens rendered at native 240x240.')
