# FM1 wTABLES RC1 — 0.83.0-rc1

This is the source used for the RC1 firmware. It includes the uploaded TEST83 engine changes and the approved six-second layered-waveform splash, with no release label in the artwork.

Current build instructions: [README-RC1.md](README-RC1.md) and `build-RC1-linux.sh`. Current release details and validation: [repository README](../README.md) and [BUILD-VALIDATION.md](../BUILD-VALIDATION.md). The TEST82/TEST83 notes below are retained as historical context; their older version strings and default-voice descriptions do not override the current RC1 instructions.

---

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

## TEST83 changes (this tree)

- Eight voices is now the **default**: a fresh settings block and all 29 factory presets start at `CORE_STARTUP_VOICES` (8). The four-slot render span and the gain-headroom reference stay on `CORE_DEFAULT_VOICES` (4), so selecting fewer voices still costs less and the -6 dB headroom above four voices is unchanged. The unit therefore starts where it used to sit after selecting 8 by hand.
- Per-block CPU at eight voices is down **11.4%** on a host instruction-count proxy, bit-exactly: the per-sample envelope, filter and filter-mod leaves are inlined, and the oscillator struct copy for the mip crossfade is taken only while a crossfade is running.
- The voice loop now executes from **executable SRAM** (`.voice_hot`) instead of in place from the flash, as the sibling V-Station firmware does. Placement only, no arithmetic change; 8,720 of the 24,576-byte RAMTEXT window is now used.
- A latent fault fixed: the build audit's `64-bit multiplies inlined beside loads ... may corrupt a register` note covered `core_render` and `pw_bloom_branch`. Those multiplies are now in `noinline` helpers, the pattern the project already uses in `pw_core.c` and `pw_performance.c`. The audit reads clean.
- Two bit-exactness gates added to `tests/run82.sh`: a 29-preset render hash and a randomized settings/notes/CC fuzz hash, both pinned to the pre-optimization output.

See `TEST83-CPU-AND-DEFAULT-8.md` for the measurements, the SRAM constraints, what was tried and rejected, and the state of the inherited test chain.

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
