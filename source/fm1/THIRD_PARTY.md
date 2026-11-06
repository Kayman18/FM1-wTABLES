# Attribution and scope

FM1 platform: fm1-base and felucca-bsp; retain base/LICENSING.md, base/CREDITS.md and their license files.

Waveform data: Adventure Kid Waveforms (AKWF-FREE), CC0-1.0, https://github.com/KristofferKarlAxelEkstrand/AKWF-FREE . TEST2 uses 64 frame slots selected from distinct source cycles. Source WAVs and exact URLs are retained under wave_sources/; build_test_tables.py resamples to 256 samples, removes DC, aligns fundamental phase and peak-normalizes offline. These are test waveforms, not extracted Disting factory waveforms.

Oscillator kernel: C translation recovered from Expert Sleepers disting EX commercial firmware 1.26.1, virtual addresses 0x9D029F4C..0x9D02A090 and its boundary branches 0x9D02A7E8..0x9D02A804. No license to the commercial firmware is inferred from the separate MIT-licensed open-source disting EX framework. This is an experimental interoperability test, not an official Expert Sleepers product or a complete Poly Wavetable port. Do not label the recovered kernel MIT or MPL on the basis of the platform's license.

Fonts: Barlow, SIL Open Font License; base/assets/fonts/Barlow-OFL.txt.

SDK packaging files: JieLi AC79NN_SDK_V1.2.1; Apache-2.0, base/LICENSES/Apache-2.0.txt. Included files are byte-identical to the reference hashes in base/tools/build.py.

TEST3 primary envelope: coefficient helper recovered from 0x9D0E8058, time/shape tables from 0x9D15E338..0x9D15E4B4 and stage transitions in Poly Wavetable's main render. Numerically adapted in app/pw_env.c for FM1 single precision and 44.1 kHz; see ../ENVELOPE.md. The same commercial-firmware licensing distinction stated for the oscillator applies. FM1 note handling and UI are integration scaffolding, not recovered Disting instrument code.

TEST4 polyphony: four-voice allocation, short voice-steal crossfades, input/channel matching, fixed-headroom mixing and CPU/XRUN display are FM1 integration code. The oscillator and primary envelope source are unchanged from TEST3. No complete Disting voice allocator or full Poly Wavetable instrument is claimed.

TEST5 filter: state-variable sample recurrence recovered from Expert Sleepers disting EX 1.26.1 Poly Wavetable render, including 0x9D02A234..0x9D02A284, coefficient selection around 0x9D029E40..0x9D029E88, cutoff and resonance initialization around 0x9D15E500..0x9D15E5E8. Adapted to 44.1 kHz in app/pw_filter.c. See ../FILTER.md for numerical and integration changes. The same commercial-firmware licensing distinction applies; no complete original filter routing or instrument is claimed.

TEST6 modulation: primary ADSR to cutoff routing, semitone depth, per-voice coefficient-table interpolation, ramp smoothing and the new control page are FM1 integration code. The recovered oscillator, primary envelope and filter arithmetic files are unchanged from TEST5. This does not claim the complete original Disting modulation matrix or depth scaling; see ../MODULATION.md.

TEST7: independent second-envelope state uses the existing recovered envelope math, with source secondary transitions traced at 0x9D02A88C..0x9D02A95C. Positive semitone cutoff routing, the six-page FM1 UI and full-bright inverted button indication are FM1 integration code. See ../ENV2.md for differences from original signed sustain and trigger behavior. Recovered arithmetic files remain unchanged from TEST6.

TEST8 LFO: sine/rate tables are produced by executing the original Poly Wavetable initializers at 0x9D15E5EC..0x9D15E68C; the phase/lookup core is recovered from 0x9D02B590..0x9D02B5D8. Original commercial-firmware licensing distinctions above apply to these recovered tables/core. 44.1 kHz increments, dezippering, limited-depth routing and fixed note-phase policy are FM1 integration adaptations. See ../LFO.md. No blanket open-source license or complete original modulation-matrix port is inferred.

TEST9 wave routes: signed envelope-wave percentage conversion and mixing structure traced at 0x9D065CD0..0x9D065D3C and 0x9D029B80..0x9D029C34. Source arithmetic files for the oscillator, envelopes, filter and LFO remain unchanged. Fixed-bank span mapping, control smoothing and the Wave Mod UI are FM1 adaptations; see ../WAVE_MOD.md. No complete original modulation matrix or factory-bank scaling is claimed.

TEST10 tracking: velocity/note wave-offset arithmetic recovered from 0x9D0EE244..0x9D0EE2DC, excluding unison spread, and cutoff tracking from the update loop at 0x9D065DA4..0x9D065E14. The source reference/normalization constants are retained in app/pw_tracking.c. Bank-span integration, smoothing and the Tracking UI are FM1 adaptations. See ../TRACKING.md; complete original tuning, velocity curves, allocator and modulation matrix are not claimed.

TEST11 LFO phase: spread and Poly/Mono/Off phase-assignment branches recovered from 0x9D09D064..0x9D09D0B8, 0x9D09D300..0x9D09D308 and 0x9D09D3C4..0x9D09D470. Source unsigned integer rounding is retained in app/pw_lfo_phase.c. Keep compatibility mode, four-voice clock integration, outgoing steal fades, dezippering and the phase-control UI are FM1 adaptations. See ../LFO_PHASE.md; full original allocation/gate behavior is not claimed.

TEST12 additionally extracts the 1024-entry pitch exponent mantissa table and reconstructs the Env2/LFO pitch mixer from the supplied Disting EX firmware. These recovered materials retain their original attribution and are not assigned a blanket open-source license here.

TEST13 exposes the recovered Disting EX attack/decay shape table and parameters for both envelopes. Original attribution and licensing qualifications above continue to apply.

TEST14 reuses the recovered Disting EX pitch exponent table with widened relative-increment arithmetic. MIDI sustain/bend ownership, tuning controls and octave handling are FM1 integration. Existing attribution and licensing qualifications continue to apply.

TEST15 recovers the original Poly Wavetable mip-selector initialization and render lookup and enables smaller source-kernel table sizes. AKWF mip preparation and crossfades are FM1 integration. The same64retained AKWF CC0 source slots are used; original Disting attribution/licensing qualifications remain.

TEST16 recovers the Disting EX Poly Wavetable stereo spreading equations, chorus lookup initializer and four-lane chorus kernel. FM1 sample-rate adaptation, pan/effect smoothing, Mix and stereo-bus integration are distinct integration code. Original attribution/licensing qualifications continue to apply.

TEST17 retains the original chorus reference kernel and adds FM1-specific continuous modulation-position interpolation and Mode 2 gain compensation. Existing source attribution/licensing qualifications remain.

TEST18 recovers the original Poly Wavetable integer unison-detune distribution and physical-voice wave-spread calculation. Four-slot group allocation, balanced pairing and live smoothing are FM1 integration. Original attribution/licensing qualifications continue to apply.

TEST19 recovers the Disting EX Poly Wavetable four-lane Stereo/Ping-pong delay routing, feedback write and additive return calculations. Two-channel pool memory, normalized injection/mix, fractional taps, crossfaded time changes and valid-history reset are FM1 adaptations. Existing attribution/licensing qualifications continue to apply.
