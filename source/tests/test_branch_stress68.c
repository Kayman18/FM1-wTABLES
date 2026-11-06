/* TEST68: event-aware Bloom generation and full-rate Clouds with the hardware-favoured 64-sample worker. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static int32_t pcm[512];
static unsigned peak;
static void render(unsigned n) {
 for(unsigned j=0;j<n;j++){
  core_render(pcm,256,4096);
  assert(!core_worker_fault());
  for(unsigned i=0;i<512;i++){
    long v=pcm[i];if(v<0)v=-v;if((unsigned)v>peak)peak=(unsigned)v;
    assert(v<=8388608);
  }
 }
}
int main(void){
 assert(VOICE_CHUNK==64u && STEAL_FADE==96u && VOICES==4);
 core_init();core_parallel_set(0);core_settings s;pw_preset_default(&s);
 s.voice_limit=4;s.unison_count=3;s.playing=0;s.bloom_mode=0;s.fx_reverb=1;s.reverb_model=3;
 s.reverb_mix=100;s.reverb_size=75;s.reverb_time_ms=18000;s.reverb_damping=45;s.reverb_depth=35;
 s.bloom_length=32;s.bloom_branches=0;s.bloom_path=85;s.bloom_mutation=100;
 s.bloom_ratchet=5;s.bloom_ratchet_chance=100;s.bloom_density=100;s.sequencer_bpm=210;
 s.env=(pw_env_params){0,5,90,55};s.env2=(pw_env_params){0,5,90,55};
 core_set(&s);render(220);s.playing=0;s.bloom_mode=1;s.bloom_branches=7;core_set(&s);s.playing=1;core_set(&s);
 /* First Bloom tick has a new gate; generation should be deferred. */
 assert(bloom.build==0);render(1);assert(bloom_running&&bloom.build==0);
 render(2400);assert(bloom.ready||bloom.valid);assert(core_primary_count()<=4);
 assert(peak>0);
 /* Mutate+branch should remain valid with alternate paths and no ratchets. */
 s.bloom_path=42;s.bloom_ratchet=0;core_set(&s);render(600);
 assert(core_primary_count()<=4);puts("PASS TEST68: branches+mutation+ratchets+Clouds100; 64-frame CPU1; event-safe tree service");
 return 0;
}
