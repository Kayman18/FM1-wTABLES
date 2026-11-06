# FM1 wTABLES RC1 — build and validation

Built on 2026-11-06 from the supplied TEST83 source, with the approved option-2 splash, the release label removed from its artwork, and a six-second startup state.

This record describes target compilation, image inspection, and host execution. **No physical FM-1 was flashed or measured.** Hardware listening, display timing, and worst-case CPU/deadline testing remain the RC acceptance step.

## Inputs and identity

| Item | Value |
| --- | --- |
| Source upload | `FM1-wTABLES-TEST83-Source.zip` |
| Source upload SHA-256 | `234b49436065a05d0f90a3c7ed0e6893f587fd3b47ba9cce308c59e2a68795da` |
| Release | `0.83.0-rc1` |
| Updater identity | `FM-1_13008300` |
| App name | `wTABLES` |
| USB/display metadata | `wTABLES RC1` |
| Compiler | `jieli-linux-toolchains-20260730.1`, Clang 4.0.1, PI32v2/r3 |
| Compiler archive SHA-256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` |
| M-Flasher base | Supplied TEST83 full image |
| Base image SHA-256 | `232a6dfdd0ade9d550acdf2daa801f2b306f4b7384d80c7ad335e73534a394ab` |

All three supplied SDK support files matched the build tool's pinned checksums. The source includes the matched Barlow fonts and licenses. The published updater is a byte-for-byte renamed copy of `pwcore-0.83.0-rc1.fwsc`.

## Built artifacts

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| [FM1-wTABLES-RC1.fwsc](firmware/FM1-wTABLES-RC1.fwsc) | 610,108 | `7ef8c023ec3a30c60a9387af907c67cbf7b090aa8cfda50cc24c933bec636de9` |
| [FM1-wTABLES-RC1-M-Flasher.bin](firmware/FM1-wTABLES-RC1-M-Flasher.bin) | 1,048,576 | `c4ee0da0e4a0a08e17635c254d93d41ea1734da11ded04a056b35110e17d6572` |
| Application payload `pwcore.bin` | 560,904 | `86572bc2b528217ea5d86cff9a33c5e1b0a2b75ddec745323e99d1ca464d554e` |

A second clean target build produced **byte-identical application, FWSC, and full M-Flasher files**. Target intermediates were removed before rebuilding; the independently built host simulator was retained. See the [reproducibility record](docs/validation/rc1-reproducibility.json).

## Target resource limits

| Region | Used | Capacity | Free |
| --- | ---: | ---: | ---: |
| Application slot | 560,904 B | 581,564 B | 20,660 B |
| Main RAM | 56,544 B | 98,304 B | 41,760 B |
| Pool | 321,988 B | 344,064 B | 22,076 B |
| Executable SRAM | 8,720 B | 24,576 B | 15,856 B |

The splash RLE stream is **2,644 bytes** and reuses the existing 240 × 240 canvas. No second framebuffer is allocated. The update loader is an 8,236-byte code image, packaged as a 6,952-byte `ota.bin`.

Both builds passed the application/RAM/pool limits, hardware-register access audit, single-precision audit, and multiply-near-load audit. Inherited aggregate-initializer and unused-variable warnings remain; no DSP files were changed merely to suppress them.

## Executable-SRAM packaging correction

TEST83 introduced a separate `.voice_hot` section and already had startup code to copy it from flash to SRAM. The inherited image builder extracted only `.text`, `.ram_text`, and `.data`; its padding left the `.voice_hot` load range filled with `0xFF`.

RC1 explicitly extracts that section and includes it at `_voice_hot_load`. The new independent ELF checker reads the linked file directly, verifies every initialized section against the final application bytes, and checks the startup copy symbols and word alignment.

| Section | Bytes | Flash load address | Runtime address |
| --- | ---: | --- | --- |
| `.text` | 552,100 | `0x02000120` | `0x02000120` |
| `.ram_text` | 2,968 | `0x02086dc4` | `0x01c00000` |
| `.voice_hot` | 5,752 | `0x0208795c` | `0x01c00b98` |
| `.data` | 84 | `0x02088fd4` | `0x01c08000` |

All four sections match exactly. The executable SRAM span is contiguous through `0x01c02210` and stays inside its 24 KiB window.

A negative-control test overwrites `.voice_hot` with `0xFF` in a temporary in-memory copy, reproducing the previous omission. The checker correctly rejects it at load address `0x0208795c`. The final deliverable bytes are never modified by this test. See [negative-control output](docs/validation/rc1-image-layout-negative-control.log).

## Firmware package integrity

- Parsed and verified FWSC identity `FM-1_13008300`.
- Verified FWSC header, table, and payload CRCs.
- Verified the packaged updater loader against the freshly built `ota.bin`.
- Decrypted the application from both FWSC and M-Flasher payloads; each matches `pwcore.bin` byte-for-byte.
- Verified the full image's directory/application CRCs, bank structure, factory-bank bytes and padding.
- Verified the full image's protected and unmanaged regions against the supplied compatible base image.

Machine-readable results: [package checks](docs/validation/rc1-packages-final.json). Complete target logs: [build 1](docs/validation/rc1-target-build-1.log), [clean build 2](docs/validation/rc1-target-build-2.log).

## Six-second startup integration

`sh tests/run_rc1_splash.sh` passed with AddressSanitizer/UndefinedBehaviorSanitizer on the host/application translation unit. Its integration harness uses the actual application, framebuffer, host platform, and audio callback. The separate engine regression below covers the DSP.

Passed cases:

- First display completes before the timestamp is recorded; early hardware initialization counts toward the same deadline.
- Splash remains active at 5,999 ms and transitions at 6,000 ms, including through the input-only service path.
- The continuous 24 MHz timer and unsigned subtraction handle counter wrap.
- The progress bar advances, and bank preparation continues while the output remains silent.
- Startup MIDI, encoder and button events are consumed; keys held through startup stay blocked until released.
- Normal notes/audio work after startup.
- Firmware-update overlay, cancellation, and five-second bootloader handoff retain priority.
- OTA UI reinitialization does not replay the splash.

Independent review found no blocking startup correctness issues and reran the already-built sanitized integration executable successfully. RLE decoding was separately checked to cover exactly 240 rows of 240 pixels with no overrun or trailing data.

The software deadline is measured from completion of the initial LCD DMA transfer. Physical panel scan timing and actual startup behavior still need on-device confirmation.

## Engine regression

`bash tests/run82.sh` passed:

- Shared eight-voice pool, ninth-note stealing, whole unison-group stealing and 64-sample steal tails.
- CC74 cutoff and CC71 resonance mappings, bounded output, and sanitizer checks.
- Heavy single-core/dual-core sample equality: both hashes `32223d73`.
- 29 factory presets × 3 voice limits × 2 unison settings: `WTGOLDEN 9fa1149245be5551`.
- 300-seed settings/note/CC/bend fuzz: `WTFUZZ d36cbbc5612ae044`.

The two output hashes match the pre-optimization TEST83 gates. No new FM-1 CPU percentage is inferred from host execution. See [engine log](docs/validation/RC1-engine-validation.log).

## Screen captures

The [README capture script](source/tests/readme-screens.txt) rendered the splash and seven normal UI pages from the actual RC1 host binary. Captures are 240 × 240 RGB images. PNG compression was optimized without changing any pixel values. The HOME capture shows the eight-voice default.

See [capture instructions](docs/GUI-CAPTURES.md) and [capture log](docs/validation/RC1-gui-captures.log).
