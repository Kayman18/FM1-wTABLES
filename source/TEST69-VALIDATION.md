# Poly Wavetable TEST69 — target build and validation

TEST69 starts from the user-tested TEST68 Bloom CPU1-64 firmware. This is an experimental expansion to six selectable voice slots, with four default. Firmware performance at 5/6 voice settings is NOT hardware-verified.

## Target hardware build

- Compiler: JieLi PI32v2, target build PASS, id FM-1_13006900, application version 0.69.0
- App code: 528,712 bytes (cap 581,564), free 52,852 bytes
- Normal RAM: 52,520 / 98,304 bytes (free 45,784 bytes)
- Audio pool RAM: 321,988 / 344,064 bytes (free 22,076 bytes)
- Dedicated 7-bank partition: 247,064 / 282,624 bytes (free 35,560 bytes)
- Seven additional AKWF banks embedded in the application: 254,963 compressed bytes
- M-Flasher full image size: 1,048,576 bytes
- Flash checks PASS: header, directory/app CRCs, encryption/decryption, 14 packed-bank CRCs and lossless decode, protected-sector preservation.
- SHA256 image: 97e6713b5729a4b5d4ac173965b322ee5d65e5a683cf5c0f4c47817fd8f3cfd2

## CPU/sound behavior

- 4 slots remain the power-on and factory-preset default.
- 5 or 6 slots selectable on GLO/K1 MAX VOICES. The choice remains when switching factory presets; switching back to 4 returns to the TEST68 4-slot render path.
- 64-sample CPU1 job size and 96-sample steal fade retained.
- Unison remains capped to 3 layers total; unison voices consume the same available slots.
- When >4 voices are selected, output gain transitions over time to a 4/N headroom ratio. This normalization is separate from GLO Synth Gain dB.
- Synth Gain dB starts at 0 dB and ALL 29 factory patches use 0 dB. Loading any factory patch resets Synth Gain to 0 dB; manual gain remains adjustable; existing user-saved gain/settings data format is unchanged.
- Stereo delay/Clouds are unchanged. Four-voice PCM test vs TEST68: exact parity, signature 9aea1e89f5a93436.

## Host tests

- `tests/run69.sh`: PASS (ASan+UBSan instrumentation) — 14 AKWF 16-frame banks, 29 gain-0 presets, stereo 750ms delay and pingpong, Clouds 60 s, 4/5/6 voice rendering, factory preset gain reset, 3-unison clamp, effect sleep/wake, 64-frame dual-core scheduling, Bloom/branch/mutate 100% under Clouds 100%, no DSP nonfinite/worker faults, single/dual parity.
- `compare-four.sh`: PASS exact sample-for-sample TEST68 vs TEST69 in four-voice reference workload.
- Tests using four-voice-only constants in the old TEST68 suite were updated for TEST69; the four-voice test conditions themselves remain unchanged.

## Hardware trial

1. Back up TEST68 and patches. Flash TEST69 with M-Flasher using the full 1 MiB `.bin`.
2. Confirm default Voice Limit = 4 and GLO Synth Gain = 0 for several factory presets.
3. Set 5 voices. Use Clouds 100%, Bloom Branch Patch, Mutate 100%, and ratchets. Monitor XRUN/clicks.
4. Set 6 voices and repeat. Observe whether clicking increases compared with 4.
5. Revert to 4 or TEST68 if audio glitches. No FM1 target timing guarantees can be inferred from host tests.
