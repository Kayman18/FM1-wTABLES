set -eu
cd "$(dirname "$0")/.."
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libpwsigned.so fm1/app/pw_filter_mod.c fm1/app/pw_filter.c
python3 tests/compare_signed_modulation.py
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest20ref.so tests/reference_test20_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest21engine.so fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c
python3 tests/compare_signed_bypass.py
cc -O2 -ffp-contract=off -fsanitize=undefined,address,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test22/test-signed tests/test_signed_modulation.c fm1/app/test_engine.c fm1/app/pw_core.c fm1/app/pw_env.c fm1/app/pw_filter.c fm1/app/pw_filter_mod.c fm1/app/pw_lfo.c fm1/app/pw_wave_mod.c fm1/app/pw_tracking.c fm1/app/pw_lfo_phase.c fm1/app/pw_env_pitch.c fm1/app/pw_performance.c fm1/app/pw_mip.c fm1/app/pw_chorus.c fm1/app/pw_stereo.c fm1/app/pw_unison.c fm1/app/pw_delay.c fm1/app/pw_velocity.c fm1/app/pw_chord.c fm1/app/pw_break.c -lm
ASAN_OPTIONS=detect_leaks=0 validation/test22/test-signed
fm1/app/build/host/pwcore_host tests/bootloader-screens.txt validation/test22 > validation/test22/bootloader-host.log
fm1/app/build/host/pwcore_host tests/bootloader-short-cancel.txt validation/test22 > validation/test22/bootloader-short-host.log
python3 tests/check_bootloader.py
fm1/app/build/host/pwcore_host tests/signed-screens.txt validation/test22
python3 tests/check_signed_audio.py
