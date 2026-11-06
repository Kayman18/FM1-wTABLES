# TEST82 Validation Record

## Host regression

`bash tests/run82.sh`

Result:

```
TEST82 heavy hashes single=32223d73 dual=32223d73
PASS TEST82: shared 8-voice pool, whole-group unison stealing, 64-sample steal tail, CC74 cutoff, CC71 resonance, 4+4 dual-core parity
```

The host test is compiled with AddressSanitizer and UndefinedBehaviorSanitizer.

## FM1 target build

Release: `0.82.0`

Build checks reported:

- app image: 557240 B
- `.data + .bss`: 56544 B / 98304 B
- linker pool: 321988 B / 344064 B
- no soft-double routines
- `.ram_text`: 929 instructions, no calls

Full 1 MiB image validation reported:

```
PASS 1 MiB: target compiler image + configured banks, full decrypted app/CRC and protection verification
App: 557240 bytes; external: 247064 bytes; internal: 308735 bytes
```

Firmware SHA256:

`170493af5ee6dc85cdbf05fa9014b5dbb3da85b7071dd772427823f2541a74b7`

## Hardware validation still required

This build must still be tested on the physical FM1 at an 8-voice setting. Prioritize worst-case sustained 8-note playback, fast stealing/retriggering, filter + wavetable modulation, matrix routing, and normal effects. A host pass does not substitute for FM1 audio deadline/underrun measurements.
