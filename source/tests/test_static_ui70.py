#!/usr/bin/env python3
"""Source-level test that idle audio changes do not trigger LCD redraw."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'fm1/app/app.c').read_text()
fr=s.split('void app_frame(void){',1)[1].split('\n#ifdef PLAT_HOST',1)[0]
assert fr.count('redraw=1;')==3, 'unexpected periodic redraw source in app_frame'
assert 'plat_cpu_pct()' in fr and 'plat_xruns()' in fr, 'diagnostics must continue sampling'
assert 'if(page==23||page==27' not in fr, 'animated page redraw still present'
assert 'core_lfo_phase()' not in fr and 'core_voice_phase(' not in fr, 'audio phase still drives redraw'
assert 'pw_tables_ready()' in fr, 'startup/bank-load readiness transition missing'
assert 'overlay_visible(' in fr and 'update_state==2' in fr, 'overlay expiry or firmware update transition missing'
assert 'const unsigned preparing_cards=1u' in s and '#define cv_begin(...) ((void)0)' in s, 'unseen detailed-page rendering not disabled'
assert 'draw_page();draw_large_panel()' in s, 'large-panel setup changed unexpectedly'
print('PASS TEST70: control-driven static redraw, overlay expiry, continuous diagnostics, obsolete hidden renderer suppressed')
