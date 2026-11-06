# FM1 Poly Wavetable TEST73 — GP5 reverb test

TEST73 adds a native float-delay port of the Valeton GP-5 V1.0.3 reverb to the latest recovered TEST72 Poly Wavetable source. All ten GP5 model tunings are selectable alongside the existing Clouds reverb in the same firmware.

This is a listening and on-device performance trial. It has been compiled for the FM1 and validated in the host simulator; it has not been flashed or timed on a physical FM1 here.

## Install and select the effect

Use `FM1-PolyWavetable-TEST73-GP5-Reverb-M-Flasher.bin` with the same M-Flasher process used for prior full-image builds. The file is a full 1,048,576-byte image. It was assembled from the uploaded TEST72 full image, preserving its bootloader, identity, calibration and unmanaged sectors. It includes the matching 14 wavetable banks.

1. Press FX repeatedly until the REVERB page appears. In the existing FX page cycle this follows RIKOSHET TONE.
2. Set K1 EFFECT to REVERB.
3. Turn K2 REVERB to select a model. Clouds remains the startup/default model and model ID 3. From Clouds, one K2 step left selects GP5 HALL; one step right selects GP5 CHURCH.
4. Start with K3 DECAY at 50 and K4 MIX around 25–35. Clouds keeps its existing decay value in milliseconds; GP5 displays Decay as a percentage.
5. The next FX page, REVERB TONE, contains Tone, High Damp, Rate and Depth for GP5. Rate is displayed in Hz, from 0.01 to 5.00. Clouds keeps its existing Space and Motion Rate controls.

The model order is GP5 AIR, GP5 ROOM, GP5 HALL, CLOUDS, GP5 CHURCH, GP5 PLATE L, GP5 PLATE, GP5 SPRING, GP5 N-STAR, GP5 DEEPSEA, GP5 SWEET (Sweet Space).

Switching models fades the wet return down, clears the shared delay memory in small chunks, then fades the new return up. Dry sound continues; the outgoing reverb tail is intentionally discarded. Only one reverb engine runs at a time. Each model's edited Decay/Tone/Damp/Rate/Depth values are remembered when switching within the current session. Mix stays common for A/B testing. This cache is not saved across power cycles.

The GP5 defaults reproduce the original models' center/default tunings. The additional FM1 Tone, Damp, Rate and Depth controls expose shared-engine parameters, so they are available even where the original pedal's model did not expose that particular knob. Tone is a low-pass cutoff control, not a frequency readout; the model's high-pass and resonance tuning remains in place.

## Suggested first hardware comparison

Use the same preset, notes, voice limit, oscillator modulation and Mix for each comparison. Start with the four-voice default, then repeat with six voices and the modulation settings that previously caused clicks. Compare Clouds with GP5 Hall, Plate L and Sweet Space. Listen to held chords, note releases, model changes and live Decay/Tone adjustments. Report the CPU/xrun indication and any clicks or broken tails.

The screen retains TEST72's static redraw behavior. The CPU display does not continuously repaint while left untouched; request a fresh panel redraw when reading it. GP5 models have different return levels, so match their audible wet level when making sound-quality comparisons. No per-model loudness normalization was added. The established 0.375 wet return trim is used for both engines, after GP5's own four-branch normalization.

## What remains from TEST72

- Four voices by default, six selectable, and the existing unison limits.
- The accepted 64-frame CPU1 job size.
- The 512-frame voice-steal fade and sequencer note-release fix.
- Play-only sequencing, Gate control, all 29 presets and 14 AKWF16 banks.
- TEST72's combined WAVE PO %, PITCH WAVE % and WAVE DEPTH optimizations.
- Static screen redraw behavior, current chorus/delay routing and original Clouds DSP behavior.

The imported TEST72 source was rebuilt before modification. Both its application and its complete 1 MiB image were byte-for-byte identical to the uploaded TEST72 files. This established the compiler, font and SDK-asset baseline.

## DSP implementation

The new engine preserves the GP5's 22.05 kHz wet core inside a 44.1 kHz input/output stream, including the original polyphase resamplers, two serial allpass diffusers, four paired feedback networks, three-tap damping filters, stereo output filters, modulation and model tunings. It streams without adding a frame or block of buffering latency.

Production delay samples are floats. This removes repeated int16 conversion, saturation and scaling from the original engine. Cached constants and coefficient updates avoid transcendental library calls in the sample loop. Tiny history values are cleared with an integer magnitude mask to prevent denormal costs. An optional quantized build is retained only for reference validation.

The production float port intentionally differs from the original at very low levels and where the original integer delay writes saturate. The quantized reference mode, not the production firmware, is the configuration that reproduces the original output bit for bit.

## Measured cost and memory

The native x86-64 benchmark uses GCC 13.3, `-O2 -ffp-contract=off`, deterministic noise, five repetitions of 4,194,304 input frames per model and process CPU time. It measures the effect sample loop, excluding initialization, control changes and the rest of the synth.

- TEST72 Clouds: median 58.895 ns per input frame.
- Ten GP5 models: 31.951–33.921 ns per input frame.
- GP5 / Clouds time ratio: 0.543–0.576, or about 42–46% less host processing time.

These are host diagnostics, not FM1 CPU percentages. They support testing the port on the device; they do not establish a target deadline margin, extra safe polyphony or a reduction of the same size in total synth CPU use.

GP5 uses 7,830 float delay cells (31,320 bytes) plus a target-confirmed 524-byte core state: 31,844 bytes, about 31.1 KiB. The prior standalone Clouds delay-plus-state footprint is 67,088 bytes, about 65.5 KiB. The combined firmware still reserves the existing shared effect pool because Clouds and delay remain available. It does not free that pool allocation merely by selecting GP5.

Target build totals:

- Application: 500,868 bytes, versus TEST72's 494,928 (+5,940).
- Normal data/BSS RAM: 53,320 / 98,304 bytes, versus 52,536 (+784).
- Audio pool: 321,988 / 344,064 bytes, unchanged.
- GP5 sample-loop code: 2,120 bytes plus a 210-byte internal stereo-filter helper. Static code size is not a cycle count.
- No libm, double-precision helpers, division helpers or memory-copy calls in the target GP5 tick/filter path. The object only refers externally to memcpy/memset for control, initialization and clearing operations.

Changing Decay currently rebuilds a small set of cached coefficients twice in the wrapper. This occurs on control changes, not every sample. The hardware knob-change test should determine whether it needs further refinement.

## Validation

The quantized C reference was compared with actual execution of the original GP5 ARM instructions. It matched all samples in ten 529,216-frame impulse responses (about 12 seconds each), plus deterministic 65,536-frame noise-and-tail probes for Air, Plate L and Sweet Space, without a timing shift.

The production float mode passed silence/reset, stereo folding/cancellation, in-place stereo, arbitrary odd/even chunk boundaries, parameter extremes, control changes and 60-second maximum-decay tails for all models. The DSP sanitizer stress test processed 2,119,743 frames with ASan/UBSan and no findings.

The full TEST73 host regression includes the inherited TEST70/71/72 tests, all 11 effect selections under four and six sounding voices, model fades, incremental clears, zero-mix sleep/wake, return to both delay styles, and exact Clouds output parity with the TEST72 reference. Single-core and parallel voice rendering produced identical PCM, with no worker faults. The real UI simulator passed 83 assertions for model selection, conditional ranges, independent control recall and zero-mix wake; its four captured screens were visually checked. Detailed logs and screenshots are supplied in the validation folder.

The full-image packager checked the decrypted application bytes, directory and payload CRCs, all 14 internal/external bank CRCs and decoded lengths, capacity limits, and every protected/unmanaged flash range against the uploaded TEST72 image.

## Rebuild

The source kit includes the complete application, tests, matched font files, minimal pinned SDK packaging assets, final image and uploaded TEST72 rollback image. The compiler is separate: use the previously uploaded `jieli-linux-toolchains-20260730.1.tar(2).xz` (SHA-256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd).

On Linux x86-64, set JIELI_TOOLCHAIN to the extracted compatible compiler directory, then run:

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/jieli-toolchain
bash build-TEST73-linux.sh release/FM1-PolyWavetable-TEST72-Combined-Mod-Optimization-M-Flasher.bin
```

The build script selects the bundled Barlow Semi Condensed 1.408 fonts and SDK assets, compiles release 0.73.0, and optionally assembles a full M-Flasher image from the supplied matching backup. Reproduction used Python 3.12.14, Pillow 12.3.0 and FreeType 2.14.3; different font-rendering versions can change the binary font data.

Run application regressions with `bash tests/run73.sh`. The bundled-dependency build script was also rerun and reproduced the final BIN byte for byte. The independent GP5 reference kit under `tests/gp5_reference/` contains the source, recorded fixtures, portable validation scripts, benchmark and result hashes. The original GP5 firmware is not needed to run comparisons against those recorded fixtures; regenerating firmware captures requires the original firmware and emulator environment documented there.

## Identity and hashes

- Firmware release: 0.73.0; package identity FM-1_13007300.
- Full TEST73 M-Flasher BIN: 47fe48a5f2466504b321c087977f287ef36c47aa6cfe222c491a47c9f509c895.
- Baseline TEST72 M-Flasher BIN: e0d831f2e93d0fa35b5c6a3556a372fafd0bdd113104d1120ae3cbc9ab4d09cd.
- Production GP5 C source: 236eca314f126c098e617f581a5183b45f7ddb979e32b4523e329d5e19a74ac1.
- Original GP5 V1.0.3 firmware used for reverse engineering: 0fa42b1bf8ef385fafede8c6f3d2da11a2a9ec2b19a4d252a739cadc19c7ed0f.
