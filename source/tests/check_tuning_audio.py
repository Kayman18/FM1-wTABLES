from pathlib import Path
import wave
import numpy as np

root = Path(__file__).resolve().parents[1]
with wave.open(str(root / 'validation/test28/tuning-wide.wav')) as w:
    sr = w.getframerate()
    a = np.frombuffer(w.readframes(w.getnframes()), '<i2').reshape(-1, 2)
x = a[int(.2 * sr):int(.7 * sr), 0].astype(float)
spectrum = np.abs(np.fft.rfft(x * np.hanning(len(x))))
frequency = np.fft.rfftfreq(len(x), 1 / sr)[spectrum.argmax()]
assert abs(frequency - 4186.009) < 4, frequency
assert np.max(abs(a.astype(int))) < 32767
assert not a[-1000:].any()
print(f'PASS: recorded +60-semitone MIDI48 fundamental {frequency:.2f} Hz, no clipping, ends in silence.')
