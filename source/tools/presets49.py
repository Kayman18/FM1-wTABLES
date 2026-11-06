from pathlib import Path
import json,copy
r=Path(__file__).resolve().parents[1];p=r/'presets/fm1-presets.json';rows=json.loads(p.read_text())[:14]
specs=[('Prism Keys','SHIMMER KEYS',8,18,[12,65,48,43],82,10,25),('Prism Drift','SLOW PAD',8,62,[80,85,105,85],72,15,-20),('Reed Choir','REED PAD',9,35,[62,65,95,70],78,13,18),('Hollow Reed','WOODY PLUCK',9,72,[5,62,10,35],70,22,-30),('Bell Garden','BELL KEYS',10,25,[1,83,0,65],104,8,12),('Glass Rain','SOFT MALLET',10,76,[2,60,0,40],88,9,-18),('Metal Bloom','METALLIC PAD',11,48,[75,95,90,85],75,17,30),('Soft Steel','METAL PLUCK',11,18,[4,67,5,42],80,12,-22),('Vowel Pad','VOCAL PAD',12,40,[78,80,100,90],93,18,22),('Vocal Pluck','VOCAL KEYS',12,68,[8,56,30,38],89,14,-26),('Pulse Keys','PULSE KEYS',13,44,[8,70,60,42],76,10,20),('Warm Tape','WARM PAD',14,50,[66,80,108,80],82,5,15),('Dust Halo','TEXTURE PAD',15,70,[85,95,100,95],68,18,24),('Dust Bells','TEXTURE PLUCK',15,12,[3,75,0,50],86,10,-20)]
for i,(name,category,bank,pos,env,cut,res,wave) in enumerate(specs):
 s=dict(bank=bank,position=pos,level=65,env=env,env2=[25,70,0,60],filter=[1,cut,res,100],filter_env_depth=10 if 'PAD' in category else 22,filter_env2_depth=0,lfo=[-20,0,0,0],wave_env_depth=wave,wave_env2_depth=0,tracking=[0,0,12,16],voice_limit=4,unison_count=1,unison_mode=0,unison_detune=10,stereo_mode=1,stereo_spread=35,coarse_tune=0,fine_tune=0,env2_pitch=0,amp_attack_shape=0,amp_decay_shape=0,env2_attack_shape=0,env2_decay_shape=0,wave_quality=1,gain_db=[9, 13, 12, 8, 8, 10, 20, 8, 13, 11, 8, 11, 14, 8][i],chorus_mode=0,chorus_mix=20,delay_mode=0,delay_time_ms=375,delay_feedback=32,delay_return_db=-12,delay_style=1,delay_mix=30)
 if 'PAD' in category:s.update(chorus_mode=1,lfo=[-35,12,3,0])
 if i in (4,5,13):s.update(delay_mode=1)
 rows.append(dict(name=name,category=category,origin='Original TEST49 preset using analytic bank '+str(bank),settings=s))
p.write_text(json.dumps(rows,indent=2)+'\n')
