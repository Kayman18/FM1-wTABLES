#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
static void frames(unsigned n){while(n){unsigned k=n>256?256:n;core_render(pcm,k,4096);for(unsigned i=0;i<k*2;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(!core_worker_fault());n-=k;}}
static core_settings setup(unsigned voices,unsigned dual){
 core_init();core_parallel_set(dual);core_settings s;pw_preset_default(&s);
 s.voice_limit=voices;s.unison_count=1;s.playing=s.bloom_mode=0;
 s.env=(pw_env_params){0,0,127,50};s.env2=s.env;
 s.fx_reverb=0;s.delay_mode=0;s.chorus_mode=0;core_set(&s);frames(44100);return s;
}
int main(void){
 for(unsigned dual=0;dual<2;dual++)for(unsigned voices=4;voices<=6;voices++){
  core_settings s=setup(voices,dual);
  voice_note_on(BLOOM_PORT,0,48,100);frames(2000);
  unsigned slot=VOICES;for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,BLOOM_PORT,0,48))slot=i;assert(slot<VOICES);
  bloom_off();frames(200);
  uint32_t left=slots[slot].main.env.remaining,left2=slots[slot].main.env2.remaining;
  assert(slots[slot].main.env.stage==PW_ENV_RELEASE&&left>0);
  float level=slots[slot].main.env.level;
  /* Both next-step and later gate-offs must leave an old release alone. */
  bloom_off();assert(slots[slot].main.env.remaining==left&&slots[slot].main.env2.remaining==left2);
  frames(200);assert(slots[slot].main.env.level<level);
  voice_note_on(BLOOM_PORT,0,60,100);frames(100);bloom_off();
  assert(slots[slot].main.env.remaining==left-300);
  frames(left+100);assert(!live(&slots[slot].main));
  /* Force a steal at the voice ceiling; old note lasts beyond 2.2 ms. */
  s=setup(voices,dual);
  for(unsigned i=0;i<voices;i++)core_midi(0,0x90,40+i*3,100);
  frames(2000);assert(core_primary_count()==voices);
  core_midi(0,0x90,88,100);
  unsigned stolen=VOICES;for(unsigned i=0;i<VOICES;i++)if(slots[i].fade)stolen=i;assert(stolen<VOICES);
  assert(slots[stolen].fade==512);frames(96);assert(slots[stolen].fade==416);
  frames(415);assert(slots[stolen].fade==1);frames(1);assert(!slots[stolen].fade);
  assert(core_primary_count()==voices);
 }
 puts("PASS TEST71: older sequencer releases finish without restarting; bounded 512-frame steals; 4/5/6 voices single/dual.");
}
