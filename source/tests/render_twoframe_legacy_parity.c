/* Compile this unchanged against the reference and mixed-bank engine. */
#ifndef PW_ENGINE_FILE
#define PW_ENGINE_FILE "../fm1/app/test_engine.c"
#endif
#include PW_ENGINE_FILE
#include <assert.h>
#include <stdio.h>
int main(void){
 int32_t pcm[512];
 core_init();core_parallel_set(1);core_settings c=st;
 c.voice_limit=6;c.unison_count=3;c.unison_detune=15;c.unison_wave_spread=65;
 c.env=(pw_env_params){1,10,90,35};c.env2=(pw_env_params){15,25,60,40};
 c.wave_env_depth=55;c.wave_env2_depth=-35;c.lfo.rate=70;c.lfo.wave=-40;c.lfo.pitch=50;
 c.tracking.velocity_wave=20;c.tracking.pitch_wave=35;
 c.matrix_source[0]=1;c.matrix_dest[0]=3;c.matrix_amount[0]=65;
 c.matrix_source[1]=2;c.matrix_dest[1]=4;c.matrix_amount[1]=-45;
 c.matrix_source[2]=3;c.matrix_dest[2]=5;c.matrix_amount[2]=40;
 core_set(&c);
 for(unsigned bank=0;bank<14;bank++){
  unsigned audible=0;
  c.bank=(uint8_t)bank;c.position=(uint8_t)(bank%3==0?0:bank%3==1?50:100);
  c.wave_quality=(uint8_t)(bank&1);core_set(&c);
  for(unsigned block=0;block<180;block++){
   if(block%9==0)core_midi(0,0x90,(uint8_t)(42+(bank+block/9)%36),105);
   if(block%9==6)core_midi(0,0x80,(uint8_t)(42+(bank+block/9)%36),0);
   core_render(pcm,256,4096);assert(!core_worker_fault()&&pw_tables_ready());
   if(!pw_tables_busy()&&pw_tables_bank()==bank)for(unsigned i=0;i<512;i++)if(pcm[i])audible++;
   assert(fwrite(pcm,1,sizeof pcm,stdout)==sizeof pcm);
  }
  assert(!pw_tables_busy()&&pw_tables_bank()==bank&&audible>1000);
 }
 return 0;
}
