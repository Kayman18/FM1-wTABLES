#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
static unsigned peak;
static void render(unsigned n) {
 for(unsigned j=0;j<n;j++){
  core_render(pcm,256,4096);assert(!core_worker_fault());
  for(unsigned i=0;i<512;i++){long v=pcm[i];if(v<0)v=-v;if((unsigned)v>peak)peak=(unsigned)v;assert(v<=8388608);}
 }
}
int main(void){
 /* TEST82: eight slots, and the steal tail is 64 samples, not 512. */
 assert(VOICE_CHUNK==64u && STEAL_FADE==64u && VOICES==8 && CORE_DEFAULT_VOICES==4u);
 for(unsigned count=4;count<=6;count++){
  core_init();core_parallel_set(1);core_settings s;pw_preset_default(&s);
  s.voice_limit=(uint8_t)count;s.unison_count=1;s.playing=0;s.bloom_mode=0;s.fx_reverb=1;s.reverb_model=3;
  s.reverb_mix=100;s.reverb_size=75;s.reverb_time_ms=18000;s.reverb_damping=45;s.reverb_depth=35;
  s.bloom_length=32;s.bloom_branches=7;s.bloom_path=85;s.bloom_mutation=100;
  s.bloom_ratchet=5;s.bloom_ratchet_chance=100;s.bloom_density=100;s.sequencer_bpm=210;
  s.env=(pw_env_params){0,5,90,55};s.env2=(pw_env_params){0,5,90,55};
  core_set(&s);render(230);s.bloom_mode=1;core_set(&s);s.playing=1;core_set(&s);
  render(1900);assert(core_primary_count()<=count && (bloom.ready||bloom.valid));assert(peak>0);
  s.bloom_path=42;s.bloom_ratchet=0;core_set(&s);render(300);assert(core_primary_count()<=count);
  printf("PASS: %u-voice Clouds100 Bloom Branch7 Mutate100 dual-core 64-frame, peak %u\n",count,peak);
 }
}
