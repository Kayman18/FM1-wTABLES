from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
lines=[line for line in (ROOT/'validation/test22/leds.log').read_text().splitlines() if line.startswith('  leds: buttons')]
all_buttons=set('FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+'.split())
dark=['HOME','FX','SEL','SEL','ENV','ENV','HOME','SAVE','HOME','PLAY','HOME','HOME','LFO','HOME','SEL','SEL','HOME','LFO','SEL','SEL','HOME']
dark+=['ENV','ENV','SEL','SEL','HOME']
dark+=['ENV','SEL','SEL','ENV','SEL','SEL','SEL','HOME']
dark+=['GLO','OCT+','OCT-','HOME']
dark+=['GLO','SEL','SEL','HOME']
dark+=['GLO','SEL','SEL','SEL','HOME']
dark+=['GLO','SEL','SEL','SEL','SEL','HOME']
dark+=['GLO','SEL','SEL','SEL','SEL','SEL','HOME']
assert len(lines)==len(dark)
for line,selected in zip(lines,dark):
 lit=set(line.split(' | ')[0].removeprefix('  leds: buttons').split())
 assert lit==all_buttons-{selected},(selected,lit,line)
assert '#' in lines[-1].split(' | ')[1]
print('PASS: full-app LED output lights all 13 unselected buttons; only held/selected button is dark across HOME, FX, both depth pages, both ADSRs, EDIT, SAVE, PLAY, LFO, Wave Mod, Tracking and LFO Phase. Keyboard LEDs retain held-key indication.')
