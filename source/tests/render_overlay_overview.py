"""Contact sheet of actual host GUI captures, not a design mockup."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import sys
r=Path(__file__).resolve().parents[1]
fonts=r/'fm1/base/assets/fonts'
def font(size,bold=False):return ImageFont.truetype(str(fonts/('BarlowSemiCondensed-Bold.ttf' if bold else 'BarlowSemiCondensed-Medium.ttf')),size)
items=[('delay-time','K2 / DELAY TIME'),('delay-mode','K1 / DELAY MODE'),('bank','PRESET / BANK'),('master','MASTER / VOLUME'),('envelope','ENV / HELD PANEL'),('combo','FX + SEL / K1'),('dynamics-amount','K2 / VELOCITY AMOUNT'),('dynamics-curve','K3 / MIDI VELOCITY'),('dynamics-panel','DYNAMICS / NORMAL PAGE')]
out=Image.new('RGB',(812,1002),'#141418');d=ImageDraw.Draw(out)
d.text((24,17),'FM1 / TEST20',font=font(28,True),fill='#f6f6f8')
d.text((24,50),'RUNNING GUI  /  NATIVE 240 x 240 CAPTURES',font=font(14),fill='#96969e')
for i,(name,label) in enumerate(items):
 x=24+(i%3)*262;y=92+(i//3)*300
 im=Image.open(r/'validation/test22'/f'release-{name}.png').convert('RGB');out.paste(im,(x,y))
 d.text((x,y+250),label,font=font(14,True),fill='#ffb600')
d.text((24,978),'HELD OR ADJUSTING  /  700 ms RETURN AFTER INPUT ENDS',font=font(13),fill='#96969e')
dest=Path(sys.argv[1]) if len(sys.argv)>1 else r/'validation/test22/overlay-overview.png'
out.save(dest)
