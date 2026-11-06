/* Integration regression for the TEST63 Clouds DSP restored to TEST66. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void render(unsigned blocks) {
 int32_t pcm[512];
 for (unsigned b=0;b<blocks;++b){
  core_render(pcm,256,4096);
  assert(!core_worker_fault());
  for(unsigned j=0;j<512;++j)assert(pcm[j]>=-8388608 && pcm[j]<=8388607);
 }
}
int main(void){
 /* TEST82: the pool is eight; CORE_DEFAULT_VOICES stays the four-voice
    render span and headroom reference. */
 assert(CORE_MAX_VOICES==8u && CORE_DEFAULT_VOICES==4u && CORE_MAX_UNISON==3u);
 assert(PW_CLOUD_FLOATS==16744u && PW_CLOUD_FLOATS<PW_DELAY_FRAMES*2u);
 core_init();core_parallel_set(0);
 core_settings s;assert(pw_preset_get(5,&s));
 s.voice_limit=4;s.fx_reverb=1;s.reverb_model=3;s.reverb_size=65;
 s.reverb_damping=45;s.reverb_time_ms=8500;s.reverb_speed=25;s.reverb_depth=23;s.reverb_mix=65;
 s.delay_mode=0;s.chorus_mode=1;core_set(&s);
 core_midi(0,0x90,64,100);render(160);
 assert(reverb_active&&reverb_ready&&reverb.used==PW_CLOUD_FLOATS&&reverb.size==65);
 assert(reverb_mix>0&&reverb.model==3);
 assert(!delay_active&&delay_mix==0);
 unsigned old_index=reverb.write_ptr;
 s.reverb_size=93;core_set(&s);render(3);
 assert(reverb_active&&reverb_ready&&reverb.size==93&&reverb.clear_left==0);
 assert(reverb.write_ptr!=old_index);/* SPACE is changed live without clearing the tail. */
 s.reverb_time_ms=30000;s.reverb_damping=90;s.reverb_depth=100;
 core_set(&s);render(40);
 assert(reverb.regen>.96f && reverb.lowpass<.4f);
 /* Effect switch retains the original shared delay / Clouds buffer policy. */
 s.fx_reverb=0;s.delay_mode=2;s.delay_mix=80;s.delay_return_db=-12;core_set(&s);render(80);
 assert(!reverb_active&&delay_active&&delay_mix>0);
 s.fx_reverb=1;s.delay_mode=0;core_set(&s);render(80);
 assert(reverb_active&&reverb_ready&&reverb_mix>0&&!delay_active);
 core_panic();render(2);assert(!core_worker_fault());
 puts("PASS TEST66: restored Clouds v63 (SPACE live, decay/mod, shared effects, four voices, no worker faults)");
}
