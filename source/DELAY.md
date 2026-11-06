# Recovered delay core and FM1 adaptation — TEST19

Reference: supplied Disting EX1.26.1, Poly Wavetable descriptor0x9D02BD74 and main renderer0x9D029930. The delay-mode labels at0x9D0DD648 are Off, Stereo and Ping-pong.

## Original source path

The four-lane delay path runs at0x9D02A4DC–0x9D02A584, with auxiliary return calculations at0x9D02B68C–0x9D02B69C. It subtracts the truncated delay sample count from the previous write index, wraps to19bits, reads four floats, and advances the write index by one. Stereo reads each lane directly; Ping-pong swaps0/1 and2/3. Each written lane is input+feedback*routedHistory. Each return is input+level*routedHistory.

Time callback0x9D065E30–0x9D065E60 converts milliseconds to48kHz samples using an integer times48. Feedback callback0x9D065EAC–0x9D065EE0 converts its percentage with the source0.01float coefficient. The original return-level callback at0x9D065E64–0x9D065EA8 uses a decibel lookup and disables the return in Off mode. Original time smoothing is present around0x9D02AA94–0x9D02AB44. These original control paths are documented for context; FM1 uses the explicit adaptations below.

pw_delay_route and pw_delay_frame retain the original four-lane routing, feedback write and additive return equations. compare_delay.py executes 6000 randomized original instruction slices in both modes, including the auxiliary outputs. Indexed reads are mapped to supplied history lanes; writes are captured from the computed source registers because the original external-memory window is not the FM1 buffer. No DSP arithmetic is replaced. Frame output/write values match with zero observed error, and original19-bit write-index progression is verified. This verifies the isolated source core, not a complete external-memory or control-system port.

## FM1 memory and timing

Production uses two float channels and24576frames, occupying 196608bytes in PLAT_POOL. The original four-lane external-memory ring supports a much longer delay. The FM1 UI exposes1–550ms within this fixed buffer, retaining the source's500ms default. Total linked pool use is 311812/344064bytes in this build, leaving 32252bytes. Steady voice count and44.1kHz sample rate are unchanged.

The target tap is milliseconds*44.1+1samples. The extra sample preserves the source's previous-write-index convention: a source integer delay D reads at age D+1 relative to the next write position. Noninteger FM1 sample positions use linear interpolation between neighboring history samples. Ring wrap uses conditional subtraction for the non-power-of-two24576-frame capacity.

Changing time crossfades old/new taps over256samples, retaining fractional interpolation at both taps. A new request waits for the current crossfade to finish. This replaces source delay-time slewing with an FM1 transition that avoids stepped read motion and a large pitch sweep. It does not claim sample-identical time-knob behavior.

The ring state tracks valid history age. Reset sets write/valid counters to zero; taps older than valid history return zero until new samples have been written. The196608-byte buffer is not wiped inside the audio callback. Tests prefill nonzero history, reset, then verify no stale replay through a complete ring wrap. Platform startup still initializes the pool once at boot.

## Gain and feedback adaptation

To avoid repeating the chorus gain-staging problem, production uses the recovered routing/feedback recurrence with normalized injection:

    write = (1-feedback)*input + feedback*routedHistory
    output = input + (routedHistory-input)*Mix

Feedback is0..0.9, Mix0..1. Both expressions are convex combinations for bounded input/history. Feedback changes therefore cannot expose an old high-gain buffer state. There is no extra wet-return boost or signal-driven compressor. At feedback0.5 the first repeat has half input amplitude, then halves each repeat. Increasing feedback lengthens the tail while reducing the initial injection. The UI Mix is a dry/wet crossfade, not the original additive dB return-level parameter.

Feedback is smoothed over a64-sample one-pole. Mix ramps at1/256per sample. Source mode changes fade to bypass, reset valid history and enable the new mode. Off fades to dry and resets history; an enabled mode at Mix0 can still record history for a later Mix increase. Panic resets immediately. Writes below1e-6 in the raw internal16-bit-scale bus are gated to zero, far below final PCM audibility, to terminate very small floating-point feedback tails.

Delay runs after the corrected chorus and before master scaling/final saturation. Stereo preserves left/right repeats; Ping-pong swaps them. Symmetric mono input therefore remains symmetric in Ping-pong; panning or stereo unison supplies the asymmetry that makes alternating repeats audible. Only the first two lanes are used on FM1.

## Verification and scope

A 3000-block delay-Off regression matches captured TEST18 audio with random unison, wave spread, chorus, stereo, both wave engines and earlier routes active. Native impulse tests cover both routing modes, feedback ratios, fractional positions, the 550 ms edge, ring wrap and exact tail silence. Continuous variable-time/feedback/mix tests remain bounded. Four aligned notes at full levels do not clip.12000sanitized combined-DSP stress blocks and immediate panic silence pass.

Full-app encoder/audio tests measure 100 ms Stereo/Ping-pong echo placement and alternating half-amplitude feedback repeats; a separate550ms case checks the maximum. Off/pure-wet leakage, clipping and zero-feedback ending silence are checked. Inherited DSP/MIDI/unison/chorus/mip/LED tests and device memory/static/package checks pass.

The user confirmed TEST17's chorus correction and TEST18 sound on FM1. Their underlying DSP code is unchanged here. New delay hardware listening and CPU/XRUN checks remain pending; host timing is not a device performance measurement. Original long-delay capacity, four hardware outputs, dB return control and100percent-feedback operation are outside this build.
