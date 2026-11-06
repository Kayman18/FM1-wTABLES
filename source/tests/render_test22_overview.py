from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import sys
r=Path(__file__).resolve().parents[1];fonts=r/'fm1/base/assets/fonts'
f=lambda n,b=False:ImageFont.truetype(str(fonts/('BarlowSemiCondensed-Bold.ttf' if b else 'BarlowSemiCondensed-Medium.ttf')),n)
im=Image.new('RGB',(812,402),'#141418');d=ImageDraw.Draw(im)
d.text((24,17),'FM1 / TEST22',font=f(28,True),fill='#f6f6f8');d.text((24,50),'BIPOLAR ENV2 + FULL SIGNED LFO PITCH / ACTUAL GUI',font=f(14),fill='#96969e')
for i,(n,label) in enumerate([('bipolar-sustain','ENV2 / NEGATIVE SUSTAIN'),('bipolar-envelope','ENV2 / BIPOLAR ENVELOPE'),('bipolar-lfo-pitch','LFO / SIGNED PITCH DEPTH')]):
 x=24+i*262;im.paste(Image.open(r/'validation/test22'/f'{n}.png').convert('RGB'),(x,92));d.text((x,342),label,font=f(14,True),fill='#ffb600')
d.text((24,378),'ENV2 SUSTAIN: -100..+100% / LFO PITCH: -1200..+1200 CENTS',font=f(13),fill='#96969e')
im.save(Path(sys.argv[1]) if len(sys.argv)>1 else r/'validation/test22/test22-overview.png')
