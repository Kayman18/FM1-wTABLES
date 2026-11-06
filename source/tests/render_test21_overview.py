"""Actual native host screens for the TEST21 release overview."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import sys
r=Path(__file__).resolve().parents[1];fonts=r/'fm1/base/assets/fonts'
def font(n,bold=False):return ImageFont.truetype(str(fonts/('BarlowSemiCondensed-Bold.ttf' if bold else 'BarlowSemiCondensed-Medium.ttf')),n)
items=[('boot-hold-5','BOOTLOADER / HOLD COUNTDOWN'),('boot-cancelled','BOOTLOADER / RELEASE TO CANCEL'),('boot-entering','BOOTLOADER / FINAL HANDOFF'),('signed-env2-overlay','ENV2 / DOWNWARD FILTER SWEEP'),('signed-lfo-wave-overlay','LFO / INVERTED WAVE SWEEP'),('signed-lfo-filter-overlay','LFO / INVERTED FILTER SWEEP')]
im=Image.new('RGB',(812,702),'#141418');d=ImageDraw.Draw(im)
d.text((24,17),'FM1 / TEST21',font=font(28,True),fill='#f6f6f8')
d.text((24,50),'BOOTLOADER SCREEN + SIGNED MODULATION / ACTUAL GUI CAPTURES',font=font(14),fill='#96969e')
for i,(name,label) in enumerate(items):
 x=24+i%3*262;y=92+i//3*300;im.paste(Image.open(r/'validation/test22'/f'{name}.png').convert('RGB'),(x,y));d.text((x,y+250),label,font=font(13,True),fill='#ffb600')
d.text((24,678),'HOLD BOTH OCTAVE BUTTONS FOR 5 SECONDS / RELEASE EITHER TO CANCEL',font=font(13),fill='#96969e')
im.save(Path(sys.argv[1]) if len(sys.argv)>1 else r/'validation/test22/test22-overview.png')
