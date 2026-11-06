# TEST66 — firmware and DSP validation

Build date: 28 October 2026 (host toolchain verification only; **real FM1 hardware not tested**).

## JieLi PI32v2 compile/link measurements

| Resource | Used | Limit | Free |
|---|---:|---:|---:|
| Application image | 528,696 B | 581,564 B | 52,868 B |
| Normal RAM (`.data`+`.bss`) | 50,440 B | 98,304 B | 47,864 B |
| Audio `.pool` RAM | 321,988 B | 344,064 B | 22,076 B |
| External wavetable partition | 247,064 B | 282,624 B | 35,560 B |
| Internal 7-bank compressed payload | 254,963 B | included in application | — |

The single-bank runtime cache occupies **65,472 B** in the on-chip audio pool. One Clouds reverb instance occupies **16,744 floats = 66,976 B** of the pre-existing shared delay/reverb buffer; it is not an additional 66,976 B reservation. The four-voice physical allocation matches the UI limit; unison is capped to three.

## Final full 1 MiB flash

- File: `FM1-PolyWavetable-TEST66-Clouds-4Voice-AKWFNames-M-Flasher.bin`
- Size: 1,048,576 B
- SHA-256: `3020bfce5e63773e41a51812e3463b086fd6f0c8aaccd15a38e791020f6eb0b4`
- App-only package: `pwcore-0.66.0.fwsc`, 610,108 B, identity `FM-1_13006600` (not the recommended full installation image).
- Full-image checks: original firmware directory/CRC, app decrypted content and CRC, 7+7 compressed bank checksums, protected/calibration sector unchanged, and regions outside intended application/external wavetable partition unchanged.

## Host tests

`bash tests/run66.sh` passes under AddressSanitizer and UndefinedBehaviorSanitizer, `-O2`, including:

- Exactly 14 UI bank names and 224 source WAVs, 16 frames × 10 mip levels and lossless codec round trip, with accurate source provenance.
- Every one of 29 factory entries valid, at least one preset per bank, unison≤3, voice cap=4.
- Four physically allocated voices; fifth note cannot add a fifth slot; single/dual-core parity.
- Stereo and ping-pong 750-ms delay, feedback, clock sync, and effects memory sharing.
- Modulation matrix priority, 32-step Bloom density/ratchet/branches and preset persistence.
- K1/K2 overlay 2800 ms; K3/K4 overlay 1400 ms; buttons 700 ms.
- 60 seconds of 44.1 kHz Clouds numerical stress with 32-bit tanks, no NaNs or runaway.
- Clouds Space changes live without clearing its tail and transition from Clouds back to audible delay; no worker faults in the host simulator.

All pass logs are supplied as `TEST66-host-regression.log` (no physical hardware CPU/load measurements are claimed).

## Limitations

The 4-voice ceiling is intended to help CPU headroom but does not establish measured hardware headroom. Test on FM1 hardware with heavy patches, Clouds at high mix/decay, unison=3, and MIDI/clock playback. User patches saved under earlier bank mappings may produce different timbres. Backup and rollback before flashing.
