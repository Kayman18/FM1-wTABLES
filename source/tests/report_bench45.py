from pathlib import Path
import re,statistics
r=Path(__file__).resolve().parents[1];before=[];after=[]
for n in range(1,4):
 for tag,dest in [('before',before),('after',after)]:dest.append([float(x) for x in re.findall(r'mode\d ([0-9.]+)ms',(r/f'TEST45-bench-{tag}-{n}.log').read_text())])
s='''# TEST45 shared-work benchmark

Median process CPU time from three alternating host runs, TEST44 versus TEST45, O2 with floating-point contraction disabled. Each case renders 1800 × 256 frames (10.449 seconds), eight-voice limit, synchronous host worker. Both FX uses INIT with chorus mode 2 at 100%, ping-pong source-additive delay at −6 dB return and 60% feedback. Timing excludes initialization. Host frequency/cache noise remains possible. These figures measure total host rendering cost, not isolated CPU0 time, and do not predict FM1 CPU percentages or establish eight-voice hardware reliability.

| Preset / mode | TEST44 CPU ms | TEST45 CPU ms | Reduction |
|---|---:|---:|---:|
'''
for i in range(20):
 x=statistics.median(r[i] for r in before);y=statistics.median(r[i] for r in after)
 name=['INIT','Fatt FM1','PlainPad FM1',"80'sStab FM1",'Both FX'][i//4];mode=['idle','one note','eight notes','Bloom sequence'][i%4]
 s+=f'| {name} / {mode} | {x:.2f} | {y:.2f} | {(1-y/x)*100:.1f}% |\n'
 if i%4==3:print(name,round(x,2),round(y,2),round((1-y/x)*100,1))
s+='\nRaw results: TEST45-bench-before-1..3.log and TEST45-bench-after-1..3.log. Reproduce with tests/profile_shared45.c, choosing tests/baseline44 engine/chorus/delay versus current files. Remaining pw_*.c modules are identical.\n'
(r/'CPU45.md').write_text(s)
