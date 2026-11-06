#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){int32_t pcm[512];double maxpeak=0;unsigned cases=0;
 for(unsigned p=1;p<PW_PRESET_COUNT;p++)for(unsigned model=0;model<4;model++){
  core_init();core_settings c;pw_preset_get(p,&c);c.fx_reverb=1;c.reverb_model=model;c.reverb_time_ms=30000;c.reverb_mix=100;c.reverb_damping=0;c.reverb_depth=100;c.voice_limit=6;c.bloom_mode=1;c.bloom_ratchet=5;c.bloom_ratchet_chance=100;c.arp_bpm=240;core_set(&c);c.playing=1;core_set(&c);
  double peak=0;unsigned clips=0;for(unsigned b=0;b<800;b++){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){double a=fabs((double)pcm[i]/8388608.);if(a>peak)peak=a;if(abs(pcm[i])>=8388607)clips++;}}
  assert(isfinite(peak)&&peak>0&&clips==0);if(peak>maxpeak)maxpeak=peak;cases++;
 }
 printf("PASS: %u preset/model cases, 100%% reverb, 30s decay, x8 ratchets at 240 BPM; no hard-clipped samples; maximum peak %.8f.\n",cases,maxpeak);
}
