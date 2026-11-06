# TEST83 — eight voices by default, and the CPU work to afford them

TEST83 starts from the TEST82 tree in this repository. TEST82 had already taken
the shared pool from six slots to eight; what it did not do was make eight
voices cheap enough to be the default, or change the default. This branch does
both.

Everything here is **bit-exact** unless a line says otherwise: the rendered
output is identical sample for sample to TEST82, and two gates in
`tests/run82.sh` enforce that.

## What changed, and what it bought

Measured as instruction counts on a host build (x86, callgrind), rendering one
256-sample block of preset 2 with eight voices sounding:

| | instructions per block |
|---|---|
| TEST82 | 1,407,461 |
| TEST83 | 1,247,700 |

**−11.4%**, with no change to the output. Cost scales almost perfectly linearly
with the number of sounding voices (TEST82: 236,150 fixed per block plus 146,414
per voice), so there was no single hot spot to delete — the saving comes from
the per-sample path itself.

### 1. The per-sample leaf functions are inlined

`pw_env_tick`, `pw_filter_tick`, `pw_filter_damping`, `pw_filter_output`,
`pw_filter_live`, `pw_filter_reset`, `pw_filter_mod_index` and `pw_filter_mod_g`
each ran as a real function call, once per voice per sample — 44,100 times a
second per voice, eight voices deep — because each lived in its own translation
unit. Their bodies are now `static inline` in their headers, with the
arithmetic and its order untouched.

Two consequences worth knowing:

- `pw_env_tick`'s stage transition (which needs the exponential-time tables)
  stays out of line as `pw_env_advance`, reached a few times per note.
- `pw_filter_mod_g`'s interpolation table had to become visible to inline the
  lookup, so `g_table` is now `pw_filter_mod_g_table`, declared in the header.
  It is still written only by `pw_filter_mod_init` and read-only afterwards.

### 2. The oscillator copy on every sample is gone

`wave_sample` took a copy of the voice's `pw_core_voice` on every sample in case
a mip crossfade needed a second oscillator. The crossfade runs for 64 samples
after a mip change and is otherwise idle, so the copy is now taken only when
`mip_fade` is set.

### 3. `voice_tracking` and `voice_pitch_units` are force-inlined

Both are per-voice caches whose call overhead exceeded the cache check itself.

### 4. The voice loop runs from executable SRAM

This is the structural one, and the only change whose benefit cannot be measured
on a host.

The FM-1's RAMTEXT window (`0x01C00000`, 24,576 bytes) held only `.ram_text`
(2,968 bytes — the flash driver). The entire voice loop executed in place from
the flash over XIP, competing for the flash interface with the wavetable reads
it performs four times per sample per voice. The sibling V-Station firmware
dedicates part of the same window to its voice loop for exactly this reason.

`.voice_hot` is now placed straight after `.ram_text` (`fm1/base/device/app.ld`),
copied to RAM at boot (`fm1/base/device/main_fm1.c`) and covered by the same
second PC-guard window (`fm1/base/bsp/chip/fm1_guard.h`). Functions opt in with
`PLAT_HOT` from `plat.h`, which is empty on host builds.

Two constraints came with it, both the same ones V-Station lives under:

- **A call from SRAM to flash is out of branch range** (`R_PI32V2_LONG_JUMP_23M2`
  cannot span `0x01C00000` to `0x02000120`). So `.voice_hot` is self-contained:
  every function the voice loop calls is in the section too. That is why
  `pw_wave_mod_position`, `pw_env_pitch_*`, `pw_lfo_*`, `pw_unison_wave`,
  `pw_velocity_gain`, `pw_tracking_cutoff_offset`, `pw_filter_mod_signed`,
  `pw_mip_select`, `pw_mip_index`, `pw_performance_units`,
  `pw_performance_increment`, `pw_core_tick`, `pw_core_tick_normalized` and the
  two deliberately-noinline 64-bit multiply helpers all carry `PLAT_HOT`.
- **Crossing the gap needs a pointer.** `core_render` stays in flash — it calls
  every effect, and pulling those in would not fit — so it reaches the voice
  loop through `render_range`, a `volatile const` function pointer the compiler
  must load rather than fold back into a direct call. The inlined
  `pw_env_tick` reaches its stage transition the same way, through
  `pw_env_advance_ptr`.

Section usage after the change:

```
.ram_text    2,968 bytes
.voice_hot   5,752 bytes
             -----
             8,720 of 24,576 (15,856 free)
```

Main RAM and the pool are untouched: `.data+.bss` 56,544 of 98,304; pool
321,988 of 344,064. Application image 557,736 of 581,564 bytes.

### 5. A latent register-corruption risk, fixed

The build audit was reporting:

```
64-bit multiplies inlined beside loads (felucca-bsp docs/CPU.md:
may corrupt a register; move them out of line): core_render, pw_bloom_branch
```

Per felucca-bsp's own CPU notes, a 32x32→64-bit multiply with a load in flight
can leave a register slightly wrong on this part, most of all beside 16-bit
loads — and one of the two sites was `core_render`, in the audio path. The
project already guards against this in `pw_core.c` and `pw_performance.c` with
`noinline` helpers; the same pattern now covers the four remaining sites (a
`mul64` helper in `test_engine.c` for the ratchet schedule and the two bloom
probability rolls, and `noinline` on `pick` in `pw_bloom.c`). The audit now
reads `no 64-bit multiply inlined beside a load`.

This is a latent-fault fix, not a speed change, and it is bit-exact: the
multiply is the same multiply.

### Tried and rejected

Hoisting the control-rate branch tests (`filter_wet`, the wave and cutoff
modulation depth tests, the `g` selection) out of the per-sample path into a
per-sample flags struct, so they were evaluated once for all eight slots rather
than once per slot. It measured **1.6% slower** — the struct build cost more
than the branches saved — so it is not in the tree.

## Eight voices by default

`CORE_DEFAULT_VOICES` was doing three separate jobs: the voice limit a fresh
settings block and the factory presets start with, the four-slot render span
(`voice_span`), and the gain-headroom reference (above four voices the engine
scales output by `CORE_DEFAULT_VOICES / voice_limit`, i.e. −6 dB at eight).

Setting that one constant to 8 would have had two unwanted effects: selecting
one or two voices would still have rendered eight slots, and the headroom would
have vanished, making everything 6 dB louder with a clipping risk.

So the default is a constant of its own:

```c
#define CORE_DEFAULT_VOICES 4u    /* render span and headroom reference */
#define CORE_STARTUP_VOICES 8u    /* the limit presets and a fresh block start with */
```

`CORE_STARTUP_VOICES` is used only for the stored default — `core_init`,
`app_init`, `voice_budget`'s zero fallback, `core_unison_effective`'s fallback,
and all 29 factory presets. The span and the headroom still reference four.

**Audible consequence:** the unit now starts where it previously sat after you
selected 8 on the MAX VOICES page by hand, including that −6 dB headroom. It is
not a new behaviour, but it is 6 dB quieter than a TEST82 unit at its own
default of 4. To go back, set `CORE_STARTUP_VOICES` to `4u`; to keep eight
voices without the headroom, change the `headroom_target` expression in
`core_render` instead — louder, with clipping to watch.

## Verification

Two new gates at the end of `tests/run82.sh`, both expecting the
pre-optimization hashes:

- `tests/golden82_hash.c` — all 29 factory presets at voice limits 4, 6 and 8,
  with and without unison, each played through a sequence that fills the pool,
  steals from it, releases notes, sends CC74 and a pitch bend. One hash of every
  sample. Expected `WTGOLDEN 9fa1149245be5551`.
- `tests/fuzz82_settings.c` — randomized settings (voice limit, unison, filter,
  envelopes and shapes, LFO, tracking, velocity, matrix routes, tuning, bank and
  position), random notes, note-offs, CC74/CC71, sustain and pitch bend, over
  single- and dual-core rendering. Expected `WTFUZZ d36cbbc5612ae044` at 300
  seeds. Also checked at 1,200 seeds: `WTFUZZ 9cc6808b224c88b6`, matching
  TEST82.

`tests/run82.sh` itself passes with its heavy hashes unchanged
(`single=32223d73 dual=32223d73`).

### The inherited test chain

`tests/run70.sh` through `run74.sh` predate TEST82 and **already fail on the
unmodified TEST82 tree**, because they assert the pre-TEST82 constants. Where
the fix was a mechanical constant refresh, it is applied here, and five tests
that failed before this branch now pass and genuinely exercise eight voices:

- `test_dual61`, `cloud_integration66`, `test_matrix_position70` — `CORE_MAX_VOICES` 6 → 8
- `test_opt68`, `test_branch_stress69` — `STEAL_FADE` 512 → 64 (TEST82 shortened the steal tail)
- `test_akwf16` — the pool, and now the default, moved

Four still fail, identically on the TEST82 tree and this one, for reasons that
predate this work and that encode superseded design rather than a mechanical
constant:

- `voices69.c` asserts a six-slot span throughout (`voice_span()==6`, limit 9
  clamping to 6)
- `voices61.c` asserts a four-voice maximum and that a limit of 8 clamps to 4
- `test_dual58.c` applies preset indices up to 40; `PW_PRESET_COUNT` is 29
- `test_dual59.c` does not compile: it references a `core_settings.arp_bpm`
  field that no longer exists

Rewriting those to the current design is a judgement call about intent, not a
refresh, so they are left as they are.

## Not verified on hardware

Every number above is a host instruction count or a link-time size. The SRAM
placement in particular cannot be measured off-chip, and it is the change most
likely to matter on the real part. The perf page is the test: at eight voices
with a dense passage, compare AUDIO AVG, AUDIO PEAK, MAX WORK and the xrun
counter against a TEST82 build.
