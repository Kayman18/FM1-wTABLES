/* Real mixed-bank audio/preview checks plus tightly allocated boundary cases.
 * The production engine is included to exercise its optimized sample path. */
#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rng=0x87a4c123u;
static uint32_t random_u32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned bounds_checks,parity_checks,preview_checks;

static void kernel_boundaries(void){
 /* Independent quotient oracle checks every possible normalized control step. */
 for(unsigned p=0;p<=3839u;p++){
  pw_core_frame_blend b=pw_core_normalized_blend((int32_t)p,2);
  assert(b.frame==0&&b.morph_q15==(uint64_t)p*32768u/3839u);
 }
 const int32_t positions[]={-100,0,1,14,15,1919,1920,3838,3839,4000};
 for(unsigned frames=2;frames<=16;frames++)for(unsigned mip=1;mip<=10;mip++){
  unsigned length=2048u>>mip;
  int16_t *table=malloc(frames*length*sizeof *table);assert(table);
  for(unsigned f=0;f<frames;f++)for(unsigned i=0;i<length;i++)table[f*length+i]=(int16_t)(-28000+(int)f*3500);
  for(unsigned k=0;k<sizeof positions/sizeof positions[0];k++){
   pw_core_voice v={random_u32(),0x1234567,positions[k],4096},original=v;
   int32_t got=pw_core_tick_normalized(&v,table,frames,mip);
   assert(v.phase==original.phase+(uint32_t)original.frequency_increment);
   assert(v.position_q8==(int32_t)pw_core_normalized_position(positions[k]));
   if(frames!=16){
    if(positions[k]<=0)assert(got==-28000*256);
    if(positions[k]>=3839)assert(got==(-28000+(int)(frames-1)*3500)*256);
    /* Linear frame constants must follow the continuous normalized sweep. */
    double ideal=-28000.0+(frames-1)*3500.0*(double)pw_core_normalized_position(positions[k])/3839.0;
    assert(fabs((double)got/256.0-ideal)<2.0);
   }
   bounds_checks++;
  }
  /* Random signed samples exercise interpolation overflow limits and wrap. */
  for(unsigned i=0;i<frames*length;i++)table[i]=(int16_t)random_u32();
  for(unsigned k=0;k<256;k++){
   pw_core_voice v={random_u32(),(int32_t)random_u32(),(int32_t)(random_u32()%4050)-100,4096};
   pw_core_voice a=v,b=v;
   int32_t x=pw_core_tick_normalized(&a,table,frames,mip);
   int32_t y=frames==16?wave_tick_unscaled(&b,table,mip):wave_tick_unscaled_normalized(&b,table,mip,frames);
   assert(x==y&&!memcmp(&a,&b,sizeof a));
   if(frames==16){pw_core_voice ref=v;assert(x==pw_core_tick(&ref,table,16,mip));assert(!memcmp(&a,&ref,sizeof a));parity_checks++;}
   /* The protected modulated-pitch path preserves phase and normalized state. */
   a=v;a.pitch_scale_q12=5201;b=a;
   x=pw_core_tick_normalized(&a,table,frames,mip);
   uint32_t step=(uint32_t)(((int64_t)b.frequency_increment*b.pitch_scale_q12)>>12);
   assert(a.phase==b.phase+step&&a.position_q8==(int32_t)pw_core_normalized_position(b.position_q8));
   if(frames==16){assert(x==pw_core_tick(&b,table,16,mip));assert(!memcmp(&a,&b,sizeof a));parity_checks++;}
   bounds_checks+=2;
  }
  free(table);
 }
 for(unsigned frames=2;frames<=16;frames++){
  pw_core_voice a={0xfffffff0u,128,3839,4096};
  assert(pw_core_tick_normalized(&a,0,frames,1)==0&&a.phase==112&&a.position_q8==3839);
 }
}

static void finish_bank(unsigned bank){
 for(unsigned n=0;n<1000&&(pw_tables_busy()||pw_tables_bank()!=bank);n++)pw_tables_service_budget(bank,1,16);
 assert(pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==bank);
}
static void real_preview_and_mips(void){
 core_init();
 for(unsigned bank=0;bank<PW_TABLE_BANKS;bank++){
  finish_bank(bank);st.bank=(uint8_t)bank;
  voice_job.tables_busy=0;voice_job.table_frame_count=pw_tables_frame_count(bank);
  assert(voice_job.table_frame_count==(bank<14?16u:2u));
  voice_frame f={0};f.active_bank=(int32_t)bank;poly_voice voice={0};
  for(unsigned quality=0;quality<2;quality++){
   st.wave_quality=(uint8_t)quality;
   for(unsigned p=0;p<=100;p++){
    st.position=(uint8_t)p;
    for(unsigned index=0;index<256;index+=17){
     pw_core_voice osc={index<<24,0,(int32_t)p*PW_FRAME_POS_MAX/100,4096};
     int32_t sample=wave_tick(&voice,&osc,&f,quality?1:0)/256;
     int32_t preview=core_wave_sample(index);
     if(bank>=14)assert(sample==preview);
     else assert(abs(sample-preview)<=1); /* Original floor/truncate convention. */
     preview_checks++;
    }
   }
  }
  if(bank>=14){
   /* Mip crossfades use the same position and advance the phase once. */
   voice_job.settings.wave_quality=1;voice=(poly_voice){0};
   voice.osc=(pw_core_voice){0xabcdefffu,0x01234567,3839,4096};
   voice.mip_ready=1;voice.mip_from=1;voice.mip_to=3;voice.mip_fade=64;
   for(unsigned step=0;step<64;step++){
    pw_core_voice a=voice.osc,b=voice.osc;
    int32_t first=pw_core_tick_normalized(&a,pw_table_mips[0][bank],2,1)/256;
    int32_t second=pw_core_tick_normalized(&b,pw_table_mips[2][bank],2,3)/256;
    int32_t expected=(first*(int32_t)voice.mip_fade+second*(64-(int32_t)voice.mip_fade))/64;
    assert(wave_sample(&voice,&f)==expected);
    assert(voice.osc.phase==a.phase&&voice.osc.position_q8==3839);
   }
  }
 }
}

static void render_scenario(unsigned parallel,int32_t *record,unsigned blocks){
 core_init();core_parallel_set(parallel);
 core_settings c=st;c.env=(pw_env_params){0,0,100,40};c.env2=(pw_env_params){5,20,80,30};
 c.wave_env_depth=70;c.wave_env2_depth=-55;c.lfo.rate=70;c.lfo.wave=65;c.lfo.pitch=50;
 c.tracking.velocity_wave=25;c.tracking.pitch_wave=-30;c.unison_wave_spread=80;
 c.voice_limit=6;c.unison_count=3;c.unison_detune=12;c.position=50;
 c.matrix_source[0]=1;c.matrix_dest[0]=3;c.matrix_amount[0]=75;
 c.matrix_source[1]=2;c.matrix_dest[1]=4;c.matrix_amount[1]=-30;
 c.matrix_source[2]=3;c.matrix_dest[2]=5;c.matrix_amount[2]=45;
 core_set(&c);
 for(unsigned block=0;block<blocks;block++){
  if(block%64==0){unsigned step=block/64;c.bank=(uint8_t)(step<11?14+step:step==11?0:13);c.position=(uint8_t)(step&1?100:0);c.wave_quality=(uint8_t)(step&1);core_set(&c);}
  if(block%17==0)core_midi(0,0x90,(uint8_t)(48+(block/17)%24),110);
  if(block%17==10)core_midi(0,0x80,(uint8_t)(48+(block/17)%24),0);
  core_render(record+block*512,256,4096);
  assert(pw_tables_ready()&&!core_worker_fault());
  for(unsigned s=0;s<512;s++)assert(record[block*512+s]>=-8388608&&record[block*512+s]<=8388607);
  for(unsigned v=0;v<6;v++){float p=core_voice_wave(v);assert(isfinite(p)&&p>=-1.0f&&p<=100.001f);}
 }
 assert(parallel?core_worker_jobs()>0:core_worker_jobs()==0);
}
static void worker_and_modulation(void){
 const unsigned blocks=13u*64u;
 int32_t *a=malloc(blocks*512*sizeof *a),*b=malloc(blocks*512*sizeof *b);assert(a&&b);
 render_scenario(0,a,blocks);render_scenario(1,b,blocks);
 assert(!memcmp(a,b,blocks*512*sizeof *a));
 unsigned audible=0;for(unsigned i=0;i<blocks*512;i++)if(a[i])audible++;
 assert(audible>10000);free(a);free(b);
}
int main(void){
 kernel_boundaries();real_preview_and_mips();worker_and_modulation();
 printf("PASS: %u bounded kernel cases, %u exact legacy kernel comparisons, %u preview checks; all 11 new banks, all mips, exact endpoints, phase wrap, six voices/three unison layers, signed matrix/envelope/LFO modulation, live bank switches and single/dual worker PCM parity.\n",bounds_checks,parity_checks,preview_checks);
 return 0;
}
