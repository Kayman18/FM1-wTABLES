# TEST70 build validation — v0.70.0

**Full flash SHA-256**: `33bb08f5c78844d8bf0771e7aa63a65ec49e8ad8d9efb3c86f637af7fa28df48`

## Target build

* Full 1 MiB M-Flasher image: **PASS** (decrypted app CRC, external and internal packed bank CRCs, protected regions preserved).
* JieLi PI32v2 application code: **519,712 bytes** of max 581,564 (**61,852 bytes free**).
* Normal RAM: **52,536 / 98,304 bytes** (**45,768 free**).
* Audio-pool: **321,988 / 344,064 bytes** (**22,076 free**).
* Wavetable flash: **247,064 / 282,624 bytes**, seven external banks. Seven more internal banks retained byte-for-byte.
* `.ram_text`, forbidden soft-double runtime, register access and image linker gates: **PASS**.

## Host regressions

* Full TEST69 audio/preset/bank/Bloom/Clouds six-voice suite under ASan/UBSan: **PASS**.
* Focused WAVE POS %, BASE WAV %, FILTER CUTOFF, FILTER RESONANCE, PITCH WAVE, WAVE DEPTH, and no-route tests at four and six voices with Clouds: **PASS**, no worker faults / integer-output overflow.
* Idle UI refresh suppression, overlay-expiry refresh, one-shot bank-readiness refresh: **PASS** source-level checks and target compilation.
* Existing three-unison cap, four default/six maximum, 64-frame CPU1 job size: **PASS**.
* Output hashes for normal no-route, filter/resonance and pitch-route signal match TEST69 on the host spotcheck. The wave-position outputs change only as the small new smoother requires.

Host benchmarks are **not target CPU utilization measurements** and may vary by CPU load. Do not infer device-level click elimination from source or host validation alone.

## Hardware testing

Start at four voices, Clouds at 100%, then set a nonzero Sloth source and matrix amount to WAVE POS % and BASE WAV %. Try filter cutoff/resonance as controls and test six voices only after four is stable. Revisit XRUN count via a deliberate control update (the screen no longer refreshes it live). Keep TEST69 as a rollback.
