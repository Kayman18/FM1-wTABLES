# TEST68 Build & validation summary



- JieLi PI32v2 target compile: PASS. Firmware application 530,472 bytes (max 581,564).
- Normal RAM: 50,456 / 98,304 bytes. Pool RAM: 321,988 / 344,064 bytes (22,076 free).
- Dedicated wavetable partition: 247,064 / 282,624 bytes (35,560 free).
- Full 1,048,576-byte M-Flasher container was rebuilt and validated: app CRC, encryption/decryption, 14 packed-bank CRCs, protected sectors and loader directory checks PASS.
- Image SHA256: `c21a52648a1a79948859080207049bd01ce57e80ac690cf3fc86c5cdb3629dc5`.

## Test validation

- Host ASan + UBSan regression suite (optimized host C at -O1) PASS: 14 banks, 29 presets, four voices, delay, Clouds 60-second test, wave-depth matrix, UI overlays, single/dual-core parity.
- Heavy Bloom performance test PASS: 7 branches, 100% mutation, 100%-chance ratchets, Clouds 100% and four-voice limit, with no host worker errors.
- 3,264 branching signature cases match TEST67 byte-for-byte including their RNG states.
- Host full-tree benchmark, original ~222.3 microseconds, optimized ~129.5 microseconds per tree (~42% less *host* time for tree generation). No real FM1 CPU percentage measured.

