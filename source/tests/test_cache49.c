#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include "../fm1/app/pw_mip_tables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
int main(void){
 core_init();assert(pw_tables_ready()&&!pw_tables_busy());

 core_init();core_settings c;pw_preset_get(2,&c);c.bloom_mode=1;c.voice_limit=6;core_set(&c);c.playing=1;core_set(&c);
 unsigned busyblocks=0,events=0;
 for(unsigned k=0;k<2400;k++){
  if(k%71==0){pw_preset_apply((k/71)%29,&c);c.bank=(k/71)%16;c.wave_quality=(k/71)%2;core_set(&c);}
  if(k%71==7){c.bank=(c.bank+3)%16;core_set(&c);}
  core_render(pcm,256,4096);assert(!core_worker_fault());assert(pw_tables_ready());assert(st.playing&&st.bloom_mode);
  if(pw_tables_busy()){busyblocks++;assert(gain==0);for(unsigned j=0;j<512;j++)assert(pcm[j]==0);assert(core_wave_sample(42)==0);}
  if(bloom_index!=255)events++;
 }
 assert(busyblocks&&events);
 for(unsigned k=0;k<150;k++)core_render(pcm,256,4096);
 assert(!pw_tables_busy()&&pw_tables_bank()==c.bank&&gain>0);
 for(unsigned b=0;b<16;b++)for(unsigned q=0;q<2;q++){
 core_init();pw_preset_default(&c);c.bank=b;c.wave_quality=q;core_set(&c);for(unsigned i=0;i<40;i++)core_render(pcm,256,4096);core_midi(0,0x90,60,100);int heard=0;for(unsigned i=0;i<100;i++){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)heard|=pcm[j]!=0;assert(pw_tables_ready()&&!core_worker_fault());}assert(heard);
 }
 puts("PASS: all 16 banks and both quality modes; 2400 rapid-switch sequencer blocks, silent partial caches, bounded loads and final recovery.");
}
