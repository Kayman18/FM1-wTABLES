#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test22
cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwcore.so native/pw_core.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwenv.so fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_original.py
python3 tests/compare_envelope.py
cc -O2 -ffp-contract=off -I fm1/base/include -o validation/test22/test-envelope tests/test_envelope.c fm1/app/pw_env.c -lm
validation/test22/test-envelope
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-engine tests/test_fm1_engine.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-engine
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-polyphony tests/test_polyphony.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-polyphony
bash fm1/build.sh --release 0.31.0 --host
fm1/app/build/host/pwcore_host tests/test22-screens.txt validation/test22
python3 tests/check_k2_audio.py
python3 tests/check_note_audio.py
fm1/app/build/host/pwcore_host tests/poly-screens.txt validation/test22
python3 tests/check_chord_audio.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwfilter.so fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_filter.py
cc -O2 -ffp-contract=off -I fm1/base/include -o validation/test22/test-filter tests/test_filter.c fm1/app/pw_filter.c -lm
validation/test22/test-filter
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-filter-engine tests/test_filter_engine.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-filter-engine
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest4ref.so tests/reference_test4_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter_mod.c fm1/app/pw_filter.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest5engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_bypass.py
fm1/app/build/host/pwcore_host tests/filter-screens.txt validation/test22
python3 tests/check_filter_audio.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest5ref.so tests/reference_test5_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest6engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_zero_depth.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-filter-mod tests/test_filter_mod.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-filter-mod
fm1/app/build/host/pwcore_host tests/mod-screens.txt validation/test22
python3 tests/check_mod_audio.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest6ref.so tests/reference_test6_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest7engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_env2_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-env2 tests/test_env2.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-env2
fm1/app/build/host/pwcore_host tests/env2-screens.txt validation/test22
python3 tests/check_env2_audio.py
fm1/app/build/host/pwcore_host tests/leds.txt validation/test22 > validation/test22/leds.log
python3 tests/check_leds.py
cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwlfo.so fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_lfo.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest7ref.so tests/reference_test7_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest8engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_lfo_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-lfo tests/test_lfo.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-lfo
fm1/app/build/host/pwcore_host tests/lfo-screens.txt validation/test22
python3 tests/check_lfo_audio.py
cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwwavemod.so fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_wave_routes.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest8ref.so tests/reference_test8_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest9engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_wave_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-wave-envelopes tests/test_wave_envelopes.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-wave-envelopes
fm1/app/build/host/pwcore_host tests/wave-screens.txt validation/test22
python3 tests/check_wave_audio.py
cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwtracking.so fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_tracking.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest9ref.so tests/reference_test9_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest10engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_tracking_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-tracking tests/test_tracking.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-tracking
fm1/app/build/host/pwcore_host tests/tracking-screens.txt validation/test22
python3 tests/check_tracking_audio.py
cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwlfophase.so fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_lfo_phase.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest10ref.so tests/reference_test10_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest11engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_phase_keep.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-phase-modes tests/test_phase_modes.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-phase-modes
fm1/app/build/host/pwcore_host tests/phase-screens.txt validation/test22
python3 tests/check_phase_audio.py

cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwenvpitch.so fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_env_pitch.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest11ref.so tests/reference_test11_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest12engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_pitch_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-env-pitch tests/test_env_pitch.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-env-pitch
fm1/app/build/host/pwcore_host tests/pitch-screens.txt validation/test22
python3 tests/check_pitch_audio.py

cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest12ref.so tests/reference_test12_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest13engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_shape_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-shapes tests/test_shapes.c fm1/app/pw_env.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-shapes
fm1/app/build/host/pwcore_host tests/shape-screens.txt validation/test22
python3 tests/check_shape_audio.py

cc -I fm1/base/include -O2 -ffp-contract=off -shared -fPIC -o native/libpwperformance.so fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_performance.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest13ref.so tests/reference_test13_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest14engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_performance_zero.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-performance tests/test_performance.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-performance
fm1/app/build/host/pwcore_host tests/performance-screens.txt validation/test22
python3 tests/check_performance_audio.py

cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwmip.so fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_mip.py
python3 tests/check_mip_tables.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest14ref.so tests/reference_test14_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest15engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_mip_legacy.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-mip tests/test_mip.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-mip
fm1/app/build/host/pwcore_host tests/mip-screens.txt validation/test22
python3 tests/check_mip_audio.py

cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwstereochorus.so fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_stereo_chorus.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest15ref.so tests/reference_test15_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest16engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_stereo_bypass.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-stereo-chorus tests/test_stereo_chorus.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-stereo-chorus
fm1/app/build/host/pwcore_host tests/stereo-screens.txt validation/test22
python3 tests/check_stereo_chorus_audio.py

python3 tests/check_chorus_smoothing.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-chorus-headroom tests/test_chorus_headroom.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-chorus-headroom

cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest16crunchref.so tests/reference_test16_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest17engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_chorus_crunch.py

cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwunison.so fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_unison.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest17ref.so tests/reference_test17_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest18engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_unison_bypass.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-unison tests/test_unison.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-unison
fm1/app/build/host/pwcore_host tests/unison-screens.txt validation/test22
python3 tests/check_unison_audio.py

cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwdelay.so fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_delay.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest18ref.so tests/reference_test18_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest19engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_delay_bypass.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-delay tests/test_delay.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-delay
fm1/app/build/host/pwcore_host tests/delay-screens.txt validation/test22
python3 tests/check_delay_audio.py

cc -O2 -ffp-contract=off -shared -fPIC -o native/libpwvelocity.so fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_velocity.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest19ref.so tests/reference_test19_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest20engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_velocity_bypass.py
python3 tests/test_velocity_engine.py
cc -O2 -fsanitize=undefined,address -o validation/test22/test-overlay tests/test_overlay.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-overlay
fm1/app/build/host/pwcore_host tests/overlay-screens.txt validation/test22
python3 tests/check_overlays.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-velocity tests/test_velocity.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-velocity
fm1/app/build/host/pwcore_host tests/overlay-audio-active.txt validation/test22
fm1/app/build/host/pwcore_host tests/overlay-audio-idle.txt validation/test22
python3 tests/check_overlay_audio.py

sh tests/test22-focused.sh

sh tests/test22-bipolar.sh
