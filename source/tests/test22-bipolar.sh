set -eu
cd "$(dirname "$0")/.."
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwenvpitch.so fm1/app/pw_env_pitch.c
python3 tests/compare_bipolar_pitch.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest21ref.so fm1/app/pw_tracking.c fm1/app/pw_env_pitch.c fm1/app/pw_core.c fm1/app/pw_unison.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c fm1/app/pw_filter_mod.c fm1/app/pw_env.c fm1/app/pw_performance.c fm1/app/pw_chorus.c fm1/app/pw_mip.c fm1/app/pw_wave_mod.c fm1/app/pw_lfo.c fm1/app/pw_lfo_phase.c fm1/app/pw_stereo.c fm1/app/pw_delay.c fm1/app/pw_filter.c tests/reference_test21_engine.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest22engine.so fm1/app/pw_tracking.c fm1/app/pw_env_pitch.c fm1/app/pw_core.c fm1/app/pw_unison.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c fm1/app/pw_filter_mod.c fm1/app/pw_env.c fm1/app/pw_performance.c fm1/app/pw_chorus.c fm1/app/pw_mip.c fm1/app/pw_wave_mod.c fm1/app/pw_lfo.c fm1/app/pw_lfo_phase.c fm1/app/pw_stereo.c fm1/app/pw_delay.c fm1/app/pw_filter.c fm1/app/test_engine.c
python3 tests/compare_bipolar_bypass.py
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-bipolar tests/test_bipolar.c fm1/app/pw_tracking.c fm1/app/pw_env_pitch.c fm1/app/pw_core.c fm1/app/pw_unison.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c fm1/app/pw_filter_mod.c fm1/app/pw_env.c fm1/app/pw_performance.c fm1/app/pw_chorus.c fm1/app/pw_mip.c fm1/app/pw_wave_mod.c fm1/app/pw_lfo.c fm1/app/pw_lfo_phase.c fm1/app/pw_stereo.c fm1/app/pw_delay.c fm1/app/pw_filter.c fm1/app/test_engine.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-bipolar
python3 tests/compare_bipolar_envelope.py
fm1/app/build/host/pwcore_host tests/bipolar-audio.txt validation/test22
python3 tests/check_bipolar_audio.py
fm1/app/build/host/pwcore_host tests/bipolar-screens.txt validation/test22
