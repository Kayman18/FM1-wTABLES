# FM1 Poly Wavetable TEST74 — GP5 wet mix

TEST74 addresses the report that GP5 reverb was too subtle even with Mix at 100. It raises the GP5 wet return and makes its Mix control a true linear dry/wet blend.

## Download and install

The release image is `FM1-PolyWavetable-TEST74-GP5-Wet-Mix-M-Flasher.bin`, a full 1,048,576-byte M-Flasher image. Use the same M-Flasher process used for TEST73. It is based on the previously delivered TEST73 full image, with bootloader, identity, calibration, protected ranges, external banks and other unmanaged sectors preserved.

The source kit includes the new image and the prior TEST73 image under `release/` for rollback. The current application is release 0.74.0, package identity FM-1_13007400, USB product PW AKWF16 TEST74 GP5.

## Controls and first listening test

Select a GP5 model on the REVERB page. From the startup Clouds selection, turn K2 one step left for GP5 Hall. GP5 Sweet is available farther right. Set K1 EFFECT to REVERB if needed.

The GP5 K4 label is now **DRY/WET %**:

| Mix | Output after the transition settles |
| --- | --- |
| 0% | Dry only |
| 25% | 75% dry gain plus 25% wet gain |
| 50% | 50% dry gain plus 50% wet gain |
| 75% | 25% dry gain plus 75% wet gain |
| 100% | Wet only; direct dry signal removed |

These percentages describe gain coefficients, not perceived loudness. Each model still has its own output level and decay character.

For the first test, use GP5 Hall or GP5 Sweet, Decay 80, and DRY/WET 75. Then turn DRY/WET to 100 to hear the reverb alone. At 100 the direct note attack disappears; the output may feel softer or more distant because it consists entirely of the reverb response. Use 60–80 if you want a strong tail while retaining some direct attack.

The next FX page retains Tone, High Damp, Rate and Depth. Tone around 80 and High Damp around 20–30 are suggested starting points for an audible tail. The model selector still remembers each model's controls within the session.

Clouds retains its existing MIX % return behavior. The new true dry/wet control applies to GP5 models.

## What changed in the audio path

TEST73 GP5:

```text
output = dry + wet * (0.375 * mix)
```

TEST74 GP5:

```text
output = dry * (1 - mix) + wet * mix
```

Here `mix` runs from 0 to 1, and `wet` already includes the GP5 core's original four-branch normalization. The host return multiplier increased from 0.375 to 1.0. At a matching nonzero Mix setting, the wet component before the output soft knee is 2.6667 times its prior amplitude, or approximately +8.52 dB. The complete output is not guaranteed to become +8.52 dB louder, because the dry contribution now falls as Mix increases.

The tank receives the full mono fold of the synth signal before the output blend. Turning Mix therefore does not lower the input feeding the reverb while it is processing. The GP5 core, resamplers, model tunings and delay memory are byte-identical to TEST73.

The existing ramp is retained: a full 0-to-100 transition takes 256 input frames, approximately 5.80 ms at 44.1 kHz. Model and effect changes fade through dry audio before shared memory is reassigned and cleared incrementally. A zero Mix still lets the reverb sleep after its fade; waking clears and fades in as before. Model switches discard the old tail, as in TEST73.

The existing output soft knee remains in place for high peaks. No automatic loudness normalization or extra reverb stage has been added.

## Scope and resources

The application changes are limited to the GP5 output mix, its K4 label, and the TEST74 USB/version identity. The four-voice default, six-voice option, 64-frame worker jobs, voice-steal fade, sequencer fixes, presets, banks and modulation optimizations remain from TEST73.

Target build results:

- Application: 500,796 bytes.
- Normal data/BSS RAM: 53,320 / 98,304 bytes, unchanged from TEST73.
- Audio pool: 321,988 / 344,064 bytes, unchanged from TEST73.
- No extra delay allocation or DSP instance.
- Target checks passed for image capacity, RAM, single-precision math and register-access policy.
- Full image checks passed for decrypted application bytes and CRCs, all 14 bank images and protected/unmanaged sector preservation.

This is a target-compiled test firmware with host validation. Physical FM1 listening and timing of TEST74 remain the next step. The prior host CPU benchmark describes the unchanged reverb core; it is not a new device CPU measurement for this mixing change.

## Source and validation

`validation/TEST74/` holds the new build log, source diff and validation results. The complete TEST73 algorithm-reference kit is retained under `tests/gp5_reference/`; its original firmware captures and core checks do not need to be regenerated for this output-mix change. Historical TEST73 documentation is in `README-TEST73.md` and `validation/TEST73/`.

Rebuild on Linux x86-64 with the compatible previously supplied JieLi PI32v2 toolchain:

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/jieli-toolchain
bash build-TEST74-linux.sh release/FM1-PolyWavetable-TEST73-GP5-Reverb-M-Flasher.bin
```

The script uses the bundled matched font and SDK assets. Run application and mixer regressions with `bash tests/run74.sh`. The compiler is separate from the source kit; its previously supplied archive is `jieli-linux-toolchains-20260730.1.tar(2).xz`, SHA-256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd.

## Completed TEST74 checks

- The full inherited TEST70/71/72/73 regression suite passed under ASan/UBSan, including Clouds golden PCM and single/parallel rendering equivalence.
- For each of the ten GP5 models, the actual core_render mixer passed dry-only at 0%, half dry/half wet at 50%, unity wet at 100%, and exact zero output when the wet callback returns zero at 100% despite a sounding input. This confirms no direct dry leakage at full wet.
- With the real GP5 algorithm, input samples, wet samples, complete tank state and delay memory were identical across nonzero Mix settings 1%, 50% and 100%. The output blend does not change excitation or reverb buildup.
- Sample-by-sample Mix, model and effect transitions preserved the 1/256 ramp and the dry/wet contract.
- All ten models passed full-wet stress with six parallel voices, +12 dB synth gain and control extremes, with finite tank/output values and bounded PCM.
- The UI simulator passed 14 assertions using real button, knob and MIDI events. The normal page and overlay correctly show DRY/WET % and 100; Clouds still shows MIX %. Captured screens were visually checked.

The new mixer test additionally enables the float-cast-overflow sanitizer. See `validation/TEST74/full-regression.log`, `mixer-final.log`, `ui-simulation.log` and `screens/` for evidence.

## SHA-256 identities

- TEST74 full M-Flasher image: 4a27bd62578dd4017b46f7d7f791959bbbc05af6e2066f8fd6db54f790c3ad3f.
- TEST74 target application: 9bdead44f0712385cd2c165120adabad1da9a4e3dfd6f8c3cdc6eb13b1561be3.
- TEST73 rollback image: 47fe48a5f2466504b321c087977f287ef36c47aa6cfe222c491a47c9f509c895.
- Unchanged GP5 core: 236eca314f126c098e617f581a5183b45f7ddb979e32b4523e329d5e19a74ac1.
