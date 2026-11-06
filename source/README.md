# FM1 PolyWavetable TEST82 — 8-Voice Dynamic Pool + MIDI Filter CC

TEST82 is an experimental FM1 PolyWavetable build aimed at a **solid 8 physical voices**, not higher headline polyphony.

## Source provenance

This tree is based on the complete editable **TEST74-derived native/two-frame staging source** recovered in `FM1-PolyWavetable-Available-Source-Backup-2026-10-29`. The exact TEST75–TEST81 application source was not present in that backup, so TEST82 should be treated as a new branch from the latest complete editable staging tree rather than as a byte-for-byte TEST81 successor.

## TEST82 changes

- Physical shared voice pool: **6 -> 8 slots**.
- Voice Limit remains user-selectable and clamps at **8**; factory/default presets remain at **4** until you select 8 for testing.
- Existing global dynamic allocator is retained: MIDI ports/channels share the same physical pool.
- Whole-note/unison-group stealing is retained; 2x unison steals two-voice groups coherently.
- Dual-core rendering naturally balances 8 slots as **4 even slots + 4 odd slots**.
- Voice-steal overlap shortened from **512 samples to 64 samples** (~1.45 ms at 44.1 kHz) to reduce transient over-rendering during dense stealing.
- Pitch-bend conversion is cached per MIDI port/channel rather than recalculated for every voice sample.
- Static pitch-unit conversion is memoized per voice.
- Velocity scale is cached per voice.
- Fully dormant slots bypass pan/LFO/filter/oscillator work earlier.
- Filter reset work is avoided while the filter is already dormant.

## MIDI filter controls

- **CC74** — Filter Cutoff / Brightness, MIDI 0..127 -> engine cutoff 0..127.
- **CC71** — Filter Resonance / Timbre, MIDI 0..127 -> engine resonance 0..100.
- These controls update the synth's global filter patch, so they work from any MIDI channel feeding the shared engine.
- Existing **CC64 sustain** and pitch bend remain intact.

## Host validation

Run:

```sh
bash tests/run82.sh
```

The TEST82 regression checks:

- 8 simultaneous notes across 8 MIDI channels share one pool.
- The 9th note steals globally while the primary count stays at 8.
- Four 2x-unison notes consume all 8 slots.
- A fifth 2x-unison note steals an entire 2-voice group.
- Steal tails expire within the new 64-sample window.
- CC74 and CC71 full-range mappings.
- Heavy 8-voice single-core vs dual-core sample-identical output.
- ASan/UBSan clean execution and 24-bit output bounds.

## Target build

Requires the JieLi Linux toolchain root containing `pi32v2/` and `common/`.

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1
./build-TEST82-linux.sh
```

This builds `fm1/app/build/pwcore.bin` and `pwcore-0.82.0.fwsc`.

To construct a verified full 1 MiB M-Flasher image while preserving the known backup's protected/identity regions:

```sh
./build-TEST82-linux.sh /path/to/known-compatible-1MiB-backup.bin FM1-PolyWavetable-TEST82-8Voice-Dynamic-CC71-74-M-Flasher.bin
```

## Hardware test target

Host tests establish allocator correctness and build integrity, but they do **not** prove real-time FM1 deadline headroom. On hardware, set **GLO -> MAX VOICES = 8** and test the heaviest presets/modulation cases. The goal is zero audible breakup, deadline misses, or underruns at 8 sustained voices. Do not raise the cap beyond 8; any extra optimization headroom should be retained for stability.
