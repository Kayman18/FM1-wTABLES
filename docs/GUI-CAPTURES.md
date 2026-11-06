# RC1 GUI captures

All PNGs in `screenshots/` are native 240 × 240 framebuffer captures from the RC1 host simulator, built from the published application source. The splash capture shows the approved layered-waveform design at the three-second midpoint. The HOME capture is taken after the six-second startup has completed. No captured screen contents are reconstructed or painted over. Hardware battery and timing values are simulated.

Reproduce from the repository root:

```sh
cd source
export X0X_FONT_DIRS="$PWD/build-deps/fonts"
export X0X_FONTSET=barlow
bash fm1/build.sh --host
mkdir -p ../docs/screenshots
fm1/app/build/host/pwcore_host tests/readme-screens.txt ../docs/screenshots
```

Requires a C compiler, Python 3, and the Python dependencies used by the font generator. See `fm1/base/tools/gen_font.py`. The approved splash artwork and its compact generated firmware header are included in the source; Pillow is only needed if regenerating that header with `python3 tools/generate_wtables_splash.py`.
