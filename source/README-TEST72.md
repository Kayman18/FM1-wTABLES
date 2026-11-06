# TEST72 — combined wave modulation optimization trial

## Reason for this trial
The user reports TEST71 clicking with six voices and simultaneous WAVE PO %, PITCH WAVE % and WAVE DEPTH % matrix routes, while four voices are fine. This is consistent with insufficient processing headroom, but no hardware deadline measurement is available to prove the cause.

## Changes
- Specialized/inlined the common oscillator path with pitch_scale_q12 == 4096. It retains the exact phase accumulation and bilinear wavetable interpolation; pitch-scaled voices use the original kernel with its isolated 64-bit multiply.
- Snapshot the table-loading state once per audio callback, avoiding a function call for every oscillator sample. Loading service finishes before voice jobs are submitted; no background cache loader is introduced.
- Combine/clamp the pitch-wave tracking depth once per sample per worker, rather than separately for every voice and fading tail. Arithmetic and order are unchanged.
- Sustain/idle envelope levels return directly in the voice renderer, avoiding redundant function calls. Moving envelopes retain the original tick routine.
- Retains six-voice ceiling/four-voice default, 64-frame CPU1 jobs, Clouds, all 29 presets/14 banks, static UI, TEST71's 512-frame graceful steal fade and sequencer-release correction, Play-only sequencer with K1 Gate %.
- No reduction in audio rate, modulation rate, wavetable resolution, interpolation, effect quality or voice count.

## Validation
- PI32v2 target compile/link and firmware checks: PASS.
- App 494928 bytes; normal RAM 52536 / 98304 bytes; audio pool 321988 / 344064 bytes.
- Complete TEST70/71 ASan/UBSan regression suite: PASS.
- New three-wave-route combination test: 4 and 6 voices, single and dual rendering, static held notes, pitch-scaled voices and dense Bloom/unison/ratchets/branches with Clouds and chorus; dynamic matrix depths and routing changes. All 12 cases pass without sanitizer findings or worker faults.
- TEST71-vs-TEST72 PCM byte comparison: identical for 5,529,600 stereo frames (44,236,800 bytes total), including simultaneous WAVE POS/BASE WAV, PITCH WAVE and WAVE DEPTH routes.
- PCM SHA-256 for both versions: `29365232079afde78e5144d7449da93c27b5349c0e9fb62bb39de81d6aa2f77f`.
- 132,000 specialized oscillator/reference comparisons: exact sample and phase/position agreement across all 11 mip sizes, signed/wrapping phase increments, position clipping and null tables.
- 1 MiB M-Flasher image: application decrypt/exact-byte comparison, directory/payload CRCs, all 14 compressed banks, protected-region and unmanaged-sector preservation: PASS against TEST70 full-image base.
- Firmware BIN SHA-256: `e0d831f2e93d0fa35b5c6a3556a372fafd0bdd113104d1120ae3cbc9ab4d09cd`.

## Limits / next hardware test
This is an optimization trial, not a confirmed cure. The hardware has not been run here. Host timings do not establish AC79 deadline headroom; no device CPU improvement or elimination of clicks is claimed.

Test the same preset and six voices with the same three matrix routes/depths and effect settings that clicked on TEST71. Four voices remain the user's known stable comparison. If six still clicks, retain the exact settings for the next investigation and compare the voice-limit page's CPU/XRUN diagnostics after reproducing it.

## Build and tests
Use `build-TEST72-linux.sh` with the matching JieLi compiler, external AC79 package files and Barlow font files as in the prior source. Run `bash tests/run72.sh` for the ASan/UBSan suite. `tests/reference71/test_engine.c` is the precise previous engine used for PCM comparison (`-DREFERENCE71` with `tests/test_combined72.c`).
The source archive omits compiler, SDK, TTFs and build/native outputs. Use `tools/make_mflasher66.py MATCHING_FULL_BACKUP.bin OUTPUT.bin` after the target build to create a full image with the bundled external banks.
