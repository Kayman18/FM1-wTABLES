# TEST66 — Poly Wavetable factory presets

**29 entries (INIT + 28 sounds). All 14 banks have factory preset coverage.**

After replacing REEDS, VOICE, METAL and BELLS, eight factory slots were revoiced for the new `fm-calm.zip` banks. The remaining factory sounds refer to valid banks; older installed user patches are not automatically remapped.

| Slot | Preset | Category | Bank |
|---:|---|---|---|
| 0 | INIT | INITIAL SOUND | `sinharm` |
| 1 | Koto | WARM UNISON | `sinharm` |
| 2 | SawChill FM1 | SOFT PLUCK | `cello` |
| 3 | PerkyMute FM1 | MUTED PLUCK | `granular` |
| 4 | Harpish FM1 | GLASS HARP | `fmsynth` |
| 5 | PlainPad FM1 | WARM PAD | `cello` |
| 6 | Prelude FM1 | SOFT KEYS | `hdrawn` |
| 7 | Frozen FM1 | AIR PAD | `flute` |
| 8 | Slowly FM1 | SLOW SWELL | `flute` |
| 9 | Prism Keys | SHIMMER KEYS | `bw_blended` |
| 10 | Prism Drift | SLOW PAD | `bw_blended` |
| 11 | Calm Tide **(revised)** | AMBIENT PAD | `fm-calm` |
| 12 | PWM Lilt **(revised)** | SOFT LEAD | `pwm-lead` |
| 13 | Karp Dusk **(revised)** | SOFT PLUCK | `karplus-mid` |
| 14 | Karp Spark **(revised)** | BRIGHT PLUCK | `karplus-hi` |
| 15 | Karp Shimmer **(revised)** | SHIMMER PAD | `karplus-hi` |
| 16 | Warm Tape | WARM PAD | `eorgan` |
| 17 | Orbit Keys | MOVING KEYS | `bw_blended` |
| 18 | Calm Haze **(revised)** | EVOLVING PAD | `fm-calm` |
| 19 | Velvet Pulse | SOFT PULSE | `bw_squ` |
| 20 | PWM Halo **(revised)** | MOVING PAD | `pwm-lead` |
| 21 | Warm Marimba | WOODY MALLET | `eorgan` |
| 22 | Silver Bow | BOWED PAD | `cello` |
| 23 | Submerged Bass | DEEP BASS | `ebass` |
| 24 | Karp Mid Air **(revised)** | SOFT KEYS | `karplus-mid` |
| 25 | Harmonic Drift | EVOLVING LEAD | `sinharm` |
| 26 | Grain Orbit | TEXTURE PAD | `granular` |
| 27 | Glass Choir | SHIMMER PAD | `fmsynth` |
| 28 | Solar Air | ATMOSPHERIC PAD | `flute` |

## Coverage

- `sinharm`: 3 preset(s)
- `cello`: 3 preset(s)
- `granular`: 2 preset(s)
- `fmsynth`: 2 preset(s)
- `hdrawn`: 1 preset(s)
- `flute`: 3 preset(s)
- `eorgan`: 2 preset(s)
- `bw_blended`: 3 preset(s)
- `ebass`: 1 preset(s)
- `fm-calm`: 2 preset(s)
- `karplus-mid`: 2 preset(s)
- `karplus-hi`: 2 preset(s)
- `bw_squ`: 1 preset(s)
- `pwm-lead`: 2 preset(s)

## Waveform sources

Each bank contains **16 different single-cycle WAV frames** taken from the indicated source directory. Source-file names and SHA-256 digests are in `data/akwf16-manifest.json`. The 10 mip levels per frame are band-limited derivatives for safe playback, not additional artist-selected frames.

| Bank index | UI label | Source archive | Source directory | Frames |
|---:|---|---|---|---:|
| 0 | `sinharm` | `AKWF.zip` | `AKWF_sinharm` | 16 |
| 1 | `cello` | `AKWF.zip` | `AKWF_cello` | 16 |
| 2 | `granular` | `AKWF.zip` | `AKWF_granular` | 16 |
| 3 | `fmsynth` | `AKWF.zip` | `AKWF_fmsynth` | 16 |
| 4 | `hdrawn` | `AKWF.zip` | `AKWF_hdrawn` | 16 |
| 5 | `flute` | `AKWF.zip` | `AKWF_flute` | 16 |
| 6 | `eorgan` | `AKWF.zip` | `AKWF_eorgan` | 16 |
| 7 | `bw_blended` | `AKWF.zip` | `AKWF_bw_blended` | 16 |
| 8 | `ebass` | `AKWF.zip` | `AKWF_ebass` | 16 |
| 9 | `fm-calm` | `fm-calm.zip` | `fm-calm` | 16 |
| 10 | `karplus-mid` | `fm-calm.zip` | `karplus-mid` | 16 |
| 11 | `karplus-hi` | `fm-calm.zip` | `karplus-hi` | 16 |
| 12 | `bw_squ` | `AKWF.zip` | `AKWF_bw_squ` | 16 |
| 13 | `pwm-lead` | `fm-calm.zip` | `pwm-lead` | 16 |
