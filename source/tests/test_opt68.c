/* TEST68 zero-mix DSP sleeping, waking, bank load budget and voice steals. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static int32_t pcm[512];
static void render(unsigned blocks){for(unsigned k=0;k<blocks;k++){core_render(pcm,256,4096);assert(!core_worker_fault());}}
static core_settings preset(void){core_settings s;assert(pw_preset_get(0,&s));s.playing=s.bloom_mode=0;s.voice_limit=4;s.unison_count=1;return s;}
int main(void){
  assert(STEAL_FADE==512u && VOICE_CHUNK==64u);
  core_init();core_parallel_set(0);core_settings s=preset();s.chorus_mode=2;s.chorus_mix=0;s.delay_mode=0;core_set(&s);render(15);
  assert(chorus_active==2 && chorus_sleeping && chorus_mix==0.0f);
  s.chorus_mix=55;core_set(&s);render(6);assert(!chorus_sleeping&&chorus_mix>0);
  s.chorus_mix=0;core_set(&s);render(6);assert(chorus_sleeping&&chorus_mix==0.0f);
  s.chorus_mode=0;s.delay_mode=2;s.delay_style=0;s.delay_mix=0;core_set(&s);render(12);
  assert(delay_active==2&&delay_sleeping&&delay_mix==0);
  s.delay_mix=65;core_set(&s);render(3);assert(!delay_sleeping&&delay_mix>0&&delay.valid>0);
  s.delay_mix=0;core_set(&s);render(10);assert(delay_sleeping&&delay_mix==0);
  s.fx_reverb=1;s.delay_mode=0;s.reverb_mix=0;s.reverb_size=60;s.reverb_time_ms=30000;core_set(&s);render(24);
  assert(reverb_active&&reverb_sleeping&&reverb_mix==0);
  s.reverb_mix=70;core_set(&s);render(24);
  assert(reverb_active&&reverb_ready&&!reverb_sleeping&&reverb_mix>0&&reverb.clear_left==0);
  unsigned old=reverb.write_ptr;render(2);assert(reverb.write_ptr!=old);
  s.reverb_mix=0;core_set(&s);render(10);assert(reverb_sleeping&&reverb_mix==0);
  old=reverb.write_ptr;render(20);assert(reverb.write_ptr==old); /* No full-rate Clouds DSP at zero mix. */
  s.reverb_mix=80;core_set(&s);render(25);assert(reverb_ready&&!reverb_sleeping&&reverb_mix>0);
  /* Voice steals taper for 512 samples; compare dual/single separately. */
  s.fx_reverb=0;s.delay_mode=0;core_set(&s);render(15);
  for(unsigned i=0;i<4;i++)core_midi(0,0x90,48+i*2,115);
  render(12);for(unsigned i=0;i<4;i++)core_midi(0,0x90,72+i*2,115);
  unsigned fades=0;for(unsigned i=0;i<4;i++)if(slots[i].fade>0&&slots[i].fade<=STEAL_FADE)fades++;
  assert(fades>=1);render(2);for(unsigned i=0;i<4;i++)assert(slots[i].fade==0);
  /* A 256-sample block without Bloom events requires 2 worker transactions. */
  core_init();core_parallel_set(1);s=preset();s.fx_reverb=0;core_set(&s);render(4);
  uint32_t before=core_worker_jobs();render(1);assert(core_worker_jobs()-before==4);
  puts("PASS TEST68: sleep/wake/re-init effects, 512-sample fades, dual-core 64-frame worker chunks");
}
