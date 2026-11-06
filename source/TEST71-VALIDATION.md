# TEST71 — graceful releases and Play-only sequencer

Built from the TEST71 wave-modulation optimization source trial (TEST70 baseline).

## Changes
- Voice-steal crossfades extended from 96 to 512 samples at 44.1 kHz: 2.18 ms to 11.61 ms. One bounded tail per slot; no added primary voices. Quietest released groups still have priority for stealing.
- Fixed Bloom gate-off restarting the release envelopes of every older sequencer note. Gate-off now acts only on held/sustained sequencer voices, allowing older amp and modulation envelopes to finish naturally.
- Play starts/stops Bloom from any page, including after boot and preset changes. Stopping releases notes normally. The former K1 On/Off tile is now Gate % (10–90%); K2 Length, K3 BPM, K4 Mutate retain their positions. Default remains stopped.
- Retains TEST71 velocity/unison wave-tracking cache optimization; static UI; Clouds; 64-frame CPU1 jobs; six-voice ceiling/four default; all 29 presets and 14 AKWF banks.

## Validation
- PI32v2 target compile/link and built-in image, RAM, FPU and register checks: PASS.
- App image: 494612 bytes; normal RAM: 52536 / 98304 bytes; audio pool: 321988 / 344064 bytes.
- TEST70 ASan/UBSan regression suite: PASS, including all presets, single/dual equivalence, 4/5/6 voices, branches/path, Clouds, matrix destinations, effects sleeping and static redraw.
- Dedicated TEST71 release test: PASS, 4/5/6 voices, single and dual. Old release sample counters are not reset by later gate-offs; old notes reach idle; stolen tails last exactly 512 frames.
- Scripted full-app Play/Stop and Gate editing: PASS, including stop away from sequencer page, and Gate changes while running without interrupting transport. Screenshot visually checked.
- Full M-Flasher image: 1048576 bytes; decrypt/exact application comparison, directory/payload CRCs, all 14 compressed banks, protected regions and all unmanaged sectors: PASS against the TEST70 image.
- BIN SHA-256: `a7239b4764cb5e4f1dbc3fbe4cbbc44567a5820c6bd680665890c05a4efd2274`

## Hardware test
This build has not run on a physical FM1 here. Longer crossfades retain the old oscillator longer, increasing work during steals; peak voice/tail count stays unchanged. Test dense sequences with long releases/unison and the modulation matrix. Stealing still shortens a note when the voice budget is exhausted; this is a short smoother transition, not unlimited polyphony.

## Building
Use `build-TEST71-linux.sh` with the compatible JieLi toolchain. The three AC79 package files and Barlow font files are external dependencies as in the prior source archive. Run `bash tests/run71.sh` for host DSP checks. Build the full-app host with `sh fm1/build.sh --host`, create an output directory, and run `fm1/app/build/host/pwcore_host tests/play71.txt OUTPUT_DIRECTORY`.
Use `tools/make_mflasher66.py MATCHING_FULL_BACKUP.bin OUTPUT.bin` to package the compiled app with the bundled bank partition, preserving the backup's protected sectors. Retain a hardware backup and TEST70 for rollback.
