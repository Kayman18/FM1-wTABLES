from pathlib import Path
import json
r=Path(__file__).resolve().parents[1]
records=json.loads((r/'presets/fm1-presets.json').read_text())
base=(r/'presets/default-body.txt').read_text()
lines=['#include "pw_presets.h"','void pw_preset_default(core_settings *out){*out=(core_settings){0};'+base+'}', 'int pw_preset_get(unsigned index,core_settings *out){if(!out||index>=PW_PRESET_COUNT)return 0;pw_preset_default(out);switch(index){']
for i,row in enumerate(records,1):
 lines.append('case %d:'%i)
 for k,v in row['settings'].items():
  if isinstance(v,list):
   typ={'env':'pw_env_params','env2':'pw_env_params','filter':'pw_filter_params','tracking':'pw_tracking_params','lfo':'pw_lfo_params'}[k]
   val='('+typ+'){'+','.join(map(str,v))+'}'
  else:val=str(v)
  lines.append('out->'+k+'='+val+';')
 lines.append('break;')
lines.append('default:break;}return 1;}')
for field,default in [('name','INIT'),('category','INITIAL SOUND')]:
 values=[default]+[row[field] for row in records]
 lines.append('const char *pw_preset_'+field+'(unsigned index){static const char *values[]={'+','.join(json.dumps(v) for v in values)+'};return values[index<PW_PRESET_COUNT?index:0];}')
(r/'fm1/app/pw_presets.c').write_text('\n'.join(lines)+'\n')

with (r/"fm1/app/pw_presets.c").open("a") as f:f.write('\n/* Instrument changes leave transport, pattern and performance controls intact. */\nint pw_preset_apply(unsigned index,core_settings *current){\n if(!current)return 0;\n core_settings next;if(!pw_preset_get(index,&next))return 0;\n next.voice_limit=current->voice_limit>CORE_MAX_VOICES?CORE_MAX_VOICES:current->voice_limit;\n for(unsigned r=0;r<4;r++){next.matrix_source[r]=current->matrix_source[r];next.matrix_dest[r]=current->matrix_dest[r];next.matrix_amount[r]=current->matrix_amount[r];} next.playing=current->playing;\n next.note=current->note;\n next.panel_octave=current->panel_octave;\n next.sustain_hold=current->sustain_hold;\n next.arp_mode=current->arp_mode;\n next.arp_clock=current->arp_clock;\n next.arp_bpm=current->arp_bpm;\n next.arp_octaves=current->arp_octaves;\n next.arp_gate_percent=current->arp_gate_percent;\n next.chord_shape=current->chord_shape;\n next.chord_scale=current->chord_scale;\n next.chord_key=current->chord_key;\n next.chord_inversion=current->chord_inversion;\n next.bloom_mode=current->bloom_mode;\n next.bloom_length=current->bloom_length;\n next.bloom_scale=current->bloom_scale;\n next.bloom_root=current->bloom_root;\n next.bloom_octave=current->bloom_octave;\n next.bloom_mutation=current->bloom_mutation;\n next.bloom_reseed=current->bloom_reseed;\n next.bloom_restore=current->bloom_restore;\n *current=next;return 1;\n}\n')
