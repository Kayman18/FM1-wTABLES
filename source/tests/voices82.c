#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int32_t pcm[512];

static void render_blocks(unsigned blocks){
 for(unsigned b=0;b<blocks;b++){
  core_render(pcm,256,4096);
  assert(!core_worker_fault());
  for(unsigned j=0;j<512;j++) assert(pcm[j]>=-8388608 && pcm[j]<=8388607);
 }
}

static core_settings stable8(void){
 core_settings s; pw_preset_default(&s);
 s.playing=0; s.bloom_mode=0; s.voice_limit=8; s.unison_count=1;
 s.env=(pw_env_params){0,16,127,96}; s.env2=(pw_env_params){0,20,127,96};
 s.filter=(pw_filter_params){PW_FILTER_LP,88,55,100};
 s.filter_env_depth=24; s.filter_env2_depth=-16;
 s.wave_quality=1; s.lfo.rate=34; s.lfo.wave=24; s.lfo.cutoff=18;
 s.wave_env_depth=18; s.wave_env2_depth=-12;
 s.tracking.velocity_wave=15; s.tracking.pitch_wave=-8;
 s.fx_reverb=0; s.delay_mix=0; s.chorus_mix=0;
 core_set(&s);
 return s;
}

static unsigned fade_slots(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].fade)n++;return n;}

static void test_allocator_and_cc(unsigned dual){
 core_init(); core_parallel_set(dual); core_settings s=stable8();
 assert(CORE_MAX_VOICES==8u && CORE_DEFAULT_VOICES==4u && CORE_MAX_UNISON==3u);
 assert(VOICE_CHUNK==64u && STEAL_FADE==64u && st.voice_limit==8 && voice_span()==8);
 render_blocks(140); /* allow external table loader to settle */

 /* One shared pool: eight channels can fill all eight physical voices. */
 for(unsigned ch=0;ch<8;ch++) core_midi(0,(uint8_t)(0x90u|ch),(uint8_t)(40u+ch*4u),110);
 render_blocks(4);
 assert(core_primary_count()==8 && core_held_count()==8 && voice_job.render_slots==8);
 for(unsigned ch=0;ch<8;ch++) assert(core_note_held(0,ch,40u+ch*4u));

 /* Ninth note steals globally; the oldest whole note is retired. */
 core_midi(0,0x98,76,112);
 assert(core_primary_count()==8 && core_note_held(0,8,76));
 assert(!core_note_held(0,0,40));
 assert(fade_slots()>=1);
 for(unsigned i=0;i<VOICES;i++) assert(slots[i].fade<=STEAL_FADE);
 render_blocks(2); /* 512 samples > 64-sample steal tail */
 assert(fade_slots()==0);

 /* Standard MIDI controls are global synth controls, from any MIDI channel. */
 core_midi(0,0xB3,74,0); assert(st.filter.cutoff==0);
 core_midi(1,0xB7,74,127); assert(st.filter.cutoff==127);
 core_midi(2,0xBF,71,0); assert(st.filter.resonance==0);
 core_midi(0,0xB2,71,64); assert(st.filter.resonance==50);
 core_midi(1,0xB5,71,127); assert(st.filter.resonance==100);
 assert(core_primary_count()==8);

 /* Four 2x-unison notes consume exactly eight physical voices. */
 core_panic(); render_blocks(2); s=stable8(); s.unison_count=2; core_set(&s);
 for(unsigned ch=0;ch<4;ch++) core_midi(1,(uint8_t)(0x90u|ch),(uint8_t)(48u+ch*5u),108);
 render_blocks(3);
 assert(core_primary_count()==8);
 for(unsigned ch=0;ch<4;ch++) assert(core_note_layers(1,ch,48u+ch*5u)==2);
 core_midi(1,0x94,72,108);
 assert(core_primary_count()==8 && core_note_layers(1,4,72)==2);
 assert(core_note_layers(1,0,48)==0); /* whole 2-voice group was stolen */
 render_blocks(4);
 assert(!core_worker_fault());
}

static uint32_t heavy_hash(unsigned dual){
 core_init(); core_parallel_set(dual); core_settings s=stable8();
 s.filter=(pw_filter_params){PW_FILTER_BP,76,82,100};
 s.lfo.cutoff=55; s.lfo.wave=42; s.lfo_pitch_cents=90;
 s.wave_env_depth=38; s.wave_env2_depth=-27;
 s.tracking.velocity_cutoff=40; s.tracking.pitch_cutoff=35;
 for(unsigned r=0;r<4;r++){
  s.matrix_source[r]=(uint8_t)(1+r*2); s.matrix_dest[r]=(uint8_t)(r==0?3:r==1?5:r==2?6:1);
  s.matrix_amount[r]=(int8_t)(r&1?-75:80);
 }
 core_set(&s); render_blocks(140);
 for(unsigned i=0;i<8;i++) core_midi((i<3)?i%3:0,(uint8_t)(0x90u|(i&15u)),(uint8_t)(38+i*6),100+i*3);
 uint32_t h=2166136261u;
 for(unsigned b=0;b<400;b++){
  if((b%31)==0) core_midi(0,(uint8_t)(0x90u|(b&7u)),(uint8_t)(45+(b%40)),105);
  if((b%47)==0) core_midi(0,(uint8_t)(0xB0u|(b&7u)),74,(uint8_t)(50+(b%78)));
  if((b%53)==0) core_midi(0,(uint8_t)(0xB0u|(b&7u)),71,(uint8_t)(25+(b%103)));
  core_render(pcm,256,4096); assert(!core_worker_fault()); assert(core_primary_count()<=8);
  for(unsigned k=0;k<512;k++){h^=(uint32_t)pcm[k];h*=16777619u;}
 }
 return h;
}

int main(void){
 test_allocator_and_cc(0);
 test_allocator_and_cc(1);
 uint32_t single=heavy_hash(0),dual=heavy_hash(1);
 printf("TEST82 heavy hashes single=%08x dual=%08x\n",single,dual);
 assert(single==dual);
 puts("PASS TEST82: shared 8-voice pool, whole-group unison stealing, 64-sample steal tail, CC74 cutoff, CC71 resonance, 4+4 dual-core parity");
 return 0;
}
