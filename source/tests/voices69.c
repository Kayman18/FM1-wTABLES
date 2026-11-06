#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static int32_t pcm[512];
static void render(unsigned blocks){for(unsigned b=0;b<blocks;b++){core_render(pcm,256,4096);assert(!core_worker_fault());for(unsigned j=0;j<512;j++)assert(pcm[j]>=-8388608 && pcm[j]<=8388607);}}
static core_settings setup(unsigned count){core_settings s;assert(pw_preset_get(0,&s));s.playing=0;s.bloom_mode=0;s.unison_count=1;s.voice_limit=(uint8_t)count;s.env=(pw_env_params){0,10,127,110};s.env2=(pw_env_params){0,20,127,110};s.fx_reverb=0;core_set(&s);return s;}
int main(void){
 assert(CORE_MAX_VOICES==6u && CORE_DEFAULT_VOICES==4u && CORE_MAX_UNISON==3u && VOICE_CHUNK==64u && STEAL_FADE==512u);
 for(unsigned i=0;i<PW_PRESET_COUNT;i++){core_settings p;assert(pw_preset_get(i,&p)&&p.voice_limit==4&&p.gain_db==0);}
 for(unsigned dual=0;dual<2;dual++){
  core_init();core_parallel_set(dual);assert(st.voice_limit==4);core_settings s=setup(4);assert(voice_span()==4);render(120);
  for(unsigned i=0;i<4;i++)core_midi(0,0x90,(uint8_t)(44+i*4),120);render(9);assert(core_primary_count()==4);assert(voice_job.render_slots==4);
  unsigned saved=core_worker_jobs();render(1);assert(core_worker_jobs()-saved==4*dual);
  core_midi(0,0x90,79,120);render(2);assert(core_primary_count()==4);
  s.gain_db=-12;assert(pw_preset_apply(28,&s)&&s.voice_limit==4&&s.gain_db==0);
  s.voice_limit=5;core_set(&s);render(10);assert(st.voice_limit==5 && voice_span()==6);
  for(unsigned i=0;i<6;i++)core_midi(0,0x90,(uint8_t)(40+i*4),118);render(8);assert(core_primary_count()==5);
  s.voice_limit=6;core_set(&s);core_midi(0,0x90,102,118);render(10);assert(voice_span()==6);
  unsigned active=core_primary_count();assert(active==6);
  unsigned upper=0;for(unsigned i=4;i<6;i++)if(live(&slots[i].main))upper++;
  assert(upper>0);assert(voice_job.render_slots==6);
  s.gain_db=15;assert(pw_preset_apply(17,&s)&&s.voice_limit==6&&s.gain_db==0);
  s.gain_db=-7;core_set(&s);assert(st.gain_db==-7); /* saved/user-edited settings retain their chosen gain */
  s.unison_count=7;core_set(&s);assert(st.unison_count==3 && core_unison_count(&st)==3);
  s.unison_count=1;core_set(&s);s.voice_limit=4;core_set(&s);render(1);assert(st.voice_limit==4 && core_primary_count()<=4 && voice_job.render_slots<=6);
  render(5);assert(voice_job.render_slots==4 && core_primary_count()<=4 && core_worker_status()!=3);
  s.voice_limit=9;core_set(&s);assert(st.voice_limit==6);
 }
 puts("PASS TEST69: four default, 5/6 selectable, 3-unison cap, 4-frame dual-core continuity, six slot render and downshift");
}
