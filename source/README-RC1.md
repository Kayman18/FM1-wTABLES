# FM1 wTABLES RC1 — source and build instructions

RC1 uses the uploaded TEST83 source as its starting point. It retains the eight
physical voice slots, eight startup voices, TEST83 executable-SRAM voice loop,
and the existing voice-count gain headroom. The release adds the approved
startup artwork and a six-second startup display. The artwork itself has no
RC1 label; the release identity appears in firmware metadata and filenames.

| Field | Value |
| --- | --- |
| Release version | `0.83.0-rc1` |
| Display version | `wTABLES RC1` |
| Application name | `wTABLES` |
| USB product | `wTABLES RC1` |
| Updater identity | `FM-1_13008300` |
| Internal build stem | `pwcore` |
| Repository | <https://github.com/Kayman18/FM1-wTABLES> |

The numeric updater identity is derived from `id_prefix = "FM-1_13"` and the
numeric version `0.83.0`. The `-rc1` suffix does not change that numeric ID.

## Dependencies

Build on Linux x86-64 with Bash, Python 3, Pillow, and the compatible JieLi
PI32v2 compiler. The matching toolchain archive is
`jieli-linux-toolchains-20260730.1.tar.xz` (Clang 4.0.1), SHA-256:

```text
a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd
```

Extract it outside this source tree, then set `JIELI_TOOLCHAIN` to the directory
containing `pi32v2/` and `common/`. The compiler is a separate dependency; it is
not embedded in this source package. The repository's
`fm1/base/tools/get_toolchain.sh` downloads the vendor's current Linux toolchain;
use the archive/version above when reproducing this release.

The matching Barlow Semi Condensed fonts, their license, and the small SDK
packaging subset are included in `build-deps/`. The wrapper selects those local
dependencies by default. `AC79_SDK` and `X0X_FONT_DIRS` may override their paths.
The historical `FONT_ASSETS_NOT_INCLUDED.txt` filename is retained, but its
contents now point to the included fonts and these current instructions.

If Pillow is not installed, a local Python environment can be prepared with:

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install Pillow
```

## Build application and FWSC

From this `source/` directory:

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/jieli-linux-toolchains-20260730.1
./build-RC1-linux.sh
```

Outputs:

- `fm1/app/build/pwcore.bin`: application payload.
- `fm1/app/build/pwcore.elf`: linked application and symbols for inspection.
- `fm1/app/build/loader/ota.bin`: RAM-resident update loader.
- `fm1/app/build/pwcore-0.83.0-rc1.fwsc`: application updater package, including
  the matching loader.

The build checks the application slot, RAM/pool limits, hardware-register access,
single-precision policy, and initialized ELF sections. In particular, it includes
and verifies `.voice_hot` at its flash load address before the startup code copies
that voice-rendering code to executable SRAM. The original TEST83 build script
did not extract that separate section; RC1 corrects the image assembly without
changing the DSP arithmetic.

## Build the full M-Flasher image

Use a known-compatible full 1 MiB image, such as the supplied TEST83 image, as the
base. A full image is needed to preserve the original boot, identity, calibration,
and private settings regions while replacing the application and factory banks.

```sh
./build-RC1-linux.sh \
  /absolute/path/to/FM1-PolyWavetable-TEST83-8Voice-Default-Optimized-M-Flasher.bin \
  /absolute/path/to/FM1-wTABLES-RC1-M-Flasher.bin
```

Omit the second argument to write `FM1-wTABLES-RC1-M-Flasher.bin` in `source/`.
The packer reads the newly built `fm1/app/build/pwcore.bin` and the two matched
bank files under `data/`. It verifies the directory and data CRCs, exact decrypted
application bytes, bank structure, and unchanged regions outside the managed
application/bank sectors. It does not install `ota.bin` into the full image;
that loader belongs to the FWSC update package.

The `.fwsc` update covers the application region and does not populate an empty
factory-bank partition. Use the full M-Flasher image when installing the matching
factory-bank data. Do not rename a full-image `.bin` to `.fwsc`.

## Checks to run

The ELF/image check runs automatically during target builds. Its explicit command
is:

```sh
python3 fm1/base/tools/check_image_layout.py \
  fm1/app/build/pwcore.elf fm1/app/build/pwcore.bin
```

The focused regression also replaces the `.voice_hot` bytes with `0xFF` in memory,
matching the old omission, and requires that the image check reject it:

```sh
python3 tests/check_rc1_image_layout.py \
  fm1/app/build/pwcore.elf fm1/app/build/pwcore.bin
```

The inherited engine regression uses a host C compiler with AddressSanitizer,
UndefinedBehaviorSanitizer, and pthread support:

```sh
bash tests/run82.sh
```

That suite covers the shared eight-voice allocator, unison-group stealing,
CC71/CC74, single/dual-core parity, and the TEST83 sample-exact golden/fuzz gates.
The regression's historical `run82` filename is retained. The pre-TEST82 tests
documented as obsolete in `TEST83-CPU-AND-DEFAULT-8.md` retain their historical
expectations.

These instructions describe the build and checks, not a claim of physical FM1
testing. Release-specific results belong in the accompanying validation record.
On-device listening and worst-case CPU/deadline checks remain the hardware
verification step for an RC.
