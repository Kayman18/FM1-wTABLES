import wave
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1]/'validation/test22'
def read(name):
 with wave.open(str(r/f'overlay-audio-{name}.wav')) as f:return np.frombuffer(f.readframes(f.getnframes()),'<i2')
a,b=read('active'),read('idle')
assert np.array_equal(a,b),(len(a),len(b));assert abs(a).max()>1000 and abs(a).max()<32767
assert not a[-44100:].any()
print('PASS: complete recorded PCM is bit-identical with overlay-only button/combination/clamped-knob/bank/SELECT/ALGO events versus idle, including USB/TRS notes, sustain and release; audible, unclipped, ending in silence.')
