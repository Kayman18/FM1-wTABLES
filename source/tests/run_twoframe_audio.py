#!/usr/bin/env python3
"""Focused host validation, not a hardware CPU benchmark or flashable build."""
from pathlib import Path
import hashlib,json,os,subprocess,sys
root=Path(__file__).resolve().parents[1]
reference=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root.parents[1]/'analog_compat_audit/recovered/project74/FM1-PolyWavetable-TEST74-GP5-Wet-Mix'
out=root/'native/twoframe-audio';out.mkdir(parents=True,exist_ok=True)
cc=os.environ.get('CC','cc')
common=['-O2','-g','-std=gnu11','-ffp-contract=off','-DPLAT_HOST','-DDC_PTHREAD','-pthread']
def sources(base):return [str(p) for p in sorted((base/'fm1/app').glob('pw_*.c'))]
def compile_test(source,dest,base=root,extra=()):
    cmd=[cc,*common,*extra,'-I',str(base/'fm1/base/include'),'-I',str(base/'fm1/app'),str(source),str(base/'tests/flash49_host.c'),str(base/'tests/dual40_pthread.c'),*sources(base),'-lm','-o',str(dest)]
    subprocess.run(cmd,cwd=base,check=True)
sanitizer=out/'bounds-and-workers'
compile_test(root/'tests/test_twoframe_audio.c',sanitizer,extra=['-fsanitize=address,undefined,float-cast-overflow','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
r=subprocess.run([str(sanitizer)],cwd=root,env=env,text=True,capture_output=True,check=True)
print(r.stdout,end='');(out/'bounds-and-workers.log').write_text(r.stdout+r.stderr)
hashes={};sizes={}
for label,base in [('reference',reference),('mixed',root)]:
    binary=out/('legacy-'+label)
    engine=base/'fm1/app/test_engine.c'
    compile_test(root/'tests/render_twoframe_legacy_parity.c',binary,base=base,extra=['-DPW_ENGINE_FILE="'+str(engine)+'"'])
    pcm=out/('legacy-'+label+'.pcm')
    with pcm.open('wb') as f:subprocess.run([str(binary)],cwd=base,stdout=f,check=True)
    data=pcm.read_bytes();hashes[label]=hashlib.sha256(data).hexdigest();sizes[label]=len(data)
assert hashes['reference']==hashes['mixed'] and sizes['reference']==sizes['mixed'],(hashes,sizes)
report={'scope':'Host-only audio/preview validation against unchanged TEST74 staging baseline; not a target release or CPU benchmark','sanitizers':'ASan + UBSan + float-cast-overflow PASS','focused_result':r.stdout.strip(),'legacy_render_bytes':sizes['mixed'],'legacy_reference_sha256':hashes['reference'],'legacy_mixed_sha256':hashes['mixed'],'legacy_pcm_parity':'PASS','position_domain':'Legacy 0..3839 unchanged; frame-count mapping only during reads','target_build':'not performed by this script'}
(out/'audio-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
