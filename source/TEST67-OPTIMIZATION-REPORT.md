# Poly Wavetable FM1 TEST67 — Optimization trial

**Versions:** TEST67 v0.67.0 (128-frame CPU1 jobs), TEST67 v0.67.1 (64-frame CPU1 comparison).
**Base:** TEST66 Clouds + AKWF16 four-voice. **Hardware run:** NOT performed. Both are complete M-Flasher 1 MiB images, made from the verified TEST66 backup; factory/custom user settings in protected sectors were preserved byte-for-byte.

## Changes

- Four primary voices and at most three unison members unchanged. The voice-steal overlap changes from 128 to 96 frames (2.90 ms to 2.18 ms); aggregate old-voice fade rendering is shortened by 25% but **peak simultaneous voice work is unchanged**.
- Clouds, delay and chorus suspend after wet/delay/chorus mix fades to zero. On re-enabling, each starts with fresh history to prevent stale delayed audio. **This intentionally discards previous tails when an effect is set to zero mix.** Clouds is cleared incrementally (2,048 samples/audio block) before becoming audible again, so recovery can incur up to ~46 ms after waking.
- Incremental bank decompress work: max **8 × 32-sample codec blocks** per audio half while voices, Bloom, or FX may be active; otherwise 16 × 32. The 16-frame cache and lossless compressed banks are unchanged. Heavy-load switches may take ~0.74 s instead of ~0.37 s before gain returns.
- Normal TEST67 reduces CPU1 worker submissions from 4 to 2 per uninterrupted 256-frame half by increasing staging to 128 samples, costing 10,000 bytes more **normal RAM**, with unchanged audio-pool usage. CPU1 64 alternative keeps the old scheduling while preserving all other optimizations.
- Animated screen redraw cap reduced from ~30 to ~15 fps when idle. Knob/button overlays and update screens still allow 33 ms redraws. Removed unconditional redraw requests from static knob overlays, without changing approved overlay durations (K1/K2 2,800 ms and K3/K4 1,400 ms).
- Clouds 32-bit float tank, 14 AKWF16 banks, 29 presets, 750 ms stereo delay, UI and MIDI handling retained.

## Target compiler accounting

| Resource | TEST66 | TEST67 128 (0.67.0) | TEST67 64 (0.67.1) | Limit |
|---|---:|---:|---:|---:|
| Application binary | 528,696 B | 529,864 B | 529,864 B | 581,564 B |
| Normal RAM | 50,440 B | 60,440 B | 50,456 B | 98,304 B |
| Audio pool | 321,988 B | 321,988 B | 321,988 B | 344,064 B |
| Wavetable partition | 247,064 B | 247,064 B | 247,064 B | 282,624 B |

Internal AKWF bank blob is **254,963 bytes**, unchanged in both builds.

## Side-by-side native host DSP results

| Scenario | TEST66 host µs/256 | TEST67 host µs/256 | Relative change |
|---|---:|---:|---:|
| 4voices-effects-off | 50.52 | 43.70 | -13.5% |
| 4voices-chorus-mix0 | 68.21 | 39.90 | -41.5% |
| 4voices-chorus-mix50 | 56.45 | 51.03 | -9.6% |
| 4voices-delay-mix0 | 77.92 | 43.82 | -43.8% |
| 4voices-delay-mix50 | 63.64 | 57.57 | -9.5% |
| 4voices-clouds-mix0 | 68.46 | 48.41 | -29.3% |
| 4voices-clouds-mix70 | 62.99 | 63.07 | +0.1% |
| 4voices-steal-ratchet | 77.01 | 73.17 | -5.0% |

These are three run medians at native x86 GCC -O2, with four notes, no worker thread, 850 blocks/trial. They are **not FM1 CPU percentages**; worker performance and UI refresh have not been measured on hardware. At active Clouds 70%, little CPU change is expected; most effect savings are at zero mix.

## Validation

- FM1 JieLi PI32v2 target build PASS for both variants: instruction, memory and soft-double checks.
- Complete 1 MiB encrypted flash-image layout and CRC: PASS for both variants; original protected sectors unchanged.
- ASan + UBSan full regression PASS on primary variant: 14 distinct AKWF16 banks, 29 presets, 4-voice limit, 3-unison maximum, 750 ms delay, 60s Clouds reverb, overlay timing, Bloom, and single/dual output parity.
- New TEST67 effects sleep/wake/reset, 96-sample steals and 128-frame worker counter PASS.
- Optional CPU1 64 variant: separate O2 Clouds integration and single/dual parity tests PASS. Full sanitizer suite not repeated for the variant because all DSP except job length is identical.

## Hardware test sequence

1. Back up your currently working firmware and saved patches. Flash the primary v0.67.0 image via M-Flasher. Verify the 14 bank labels, presets, four voices, reverb and delay.
2. Test 4 notes with Bloom ratchets + 3-unison and Clouds mix 70%, while observing XRUNs and audio clicks. Compare page 23 REC single vs dual CPU mode.
3. Switch between banks while reverb and chorus are active; bank changes may now take ~0.74 s while voices/effects are active; listen for gaps longer than expected.
4. Turn Clouds mix from 70% down to 0%, and back up. The old wet tail is cleared at zero. The wet fade re-enters after the buffer has cleared, possibly ~46 ms later.
5. If dual-core mode produces more glitches than expected, flash the **CPU1 64 comparison image v0.67.1** and retest under identical settings. It shares the exact same AKWF data and presets.
6. Roll back to TEST66 if a new problem appears. Firmware has **not been physically tested**.

## Not changed

No half-rate tail oscillator playback, no mipmap-quality reductions, no framebuffer size reduction, no change to the 4-voice limit, no split of reverb/delay buffers. Those are higher-risk changes reserved for later hardware evidence.

## SHA-256

- `FM1-PolyWavetable-TEST67-Optimized-Clouds-4Voice-M-Flasher.bin`: `aad552a4efb20de486aa4e1be15f456406f34a1c5a3310b79e96604c67e298f0`
- `FM1-PolyWavetable-TEST67-CPU1-64-Comparison-M-Flasher.bin`: `9432e264704f21b6f2de0515942cc670080e13ed8a0fea61315fabdadc29d861`

